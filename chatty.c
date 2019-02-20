/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief File principale del server ChatterBox. Contiene l'implementazione delle funzioni del file chatty.h
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 */

#define _POSIX_C_SOURCE 200809L
#include <chatty.h>

/** struttura che memorizza le statistiche del server */
struct statistics chattyStats  = {0,0,0,0,0,0,0};

/**< struttura che memorizza la configurazione del server */
config_t *conf = NULL;

/**< percorso del file contenente i parametri di configurazione del server */
char* conf_filepath = NULL;

/**< tabella hash per la memorizzazione degli utenti registrati */
icl_hash_t *users = NULL;

/**< pool di thread che soddisfano le richieste dei client, collocate in una coda condivisa nel pool */
threadpool_t *thpool = NULL;

/**< thread listener */
pthread_t listener;
/**< thread gestore dei segnali */
pthread_t sig_manager;

/**< array di mutex usate per l'accesso concorrente alla tabella hash degli utenti registrati */
pthread_mutex_t mtx_users[MAX_MTX_USR];
/**< array di mutex usate nell'invio di risposte ai client */
pthread_mutex_t mtx_req[MAX_MTX_REQ];
/**< mutex per le statistiche del server */
pthread_mutex_t mtx_stats = PTHREAD_MUTEX_INITIALIZER;
/**< mutex per la pipe usata dai thread worker */
pthread_mutex_t mtx_pipe = PTHREAD_MUTEX_INITIALIZER;

/**< pipe per la comunicazione tra listener e thread del pool */
int answers[2];

/* ----- FUNZIONI DI UTILITÀ ------ */

/**
 * @function usage
 * @brief mosta un messaggio sull'uso corretto di esecuzione del server
 * @param[in] progname nome del server
 */
static void usage(const char *progname);

/**
 * @function unlinkSocket
 * @brief elimina il socket creata in sessioni precedenti del server
 */
static void unlinkSocket();


/**
 * @function cleanup
 * @brief Libera la memoria allocata durante l'esecuzione del server
 */
void cleanup();

/**
 * @fucntion main
 * @brief Funzione principale di ChatterBox
 *
 * @return 0, in caso di successo.
 * @return <0 in caso di errore.
 */
int main(int argc, char *argv[])
{
    // mi assicuro di eliminare il socket e liberare la memoria quando il server termina
    atexit(cleanup);
    atexit(unlinkSocket);

    /* ------- Gestione dei segnali ------ */

    // affido la gestione degli altri segnali ad un thread
    pthread_attr_t attr;
    sigset_t sigset;

    // creo il set di segnali che voglio mascherare
    SYSCALL(sigemptyset(&sigset),-1, "sigemptyset in main");
    SYSCALL(sigaddset(&sigset, SIGPIPE),-1, "sigaddset SIGPIPE");
    SYSCALL(sigaddset(&sigset, SIGUSR1),-1, "sigaddset SIGUSR1");
    SYSCALL(sigaddset(&sigset, SIGQUIT),-1, "sigaddset SIGQUIT");
    SYSCALL(sigaddset(&sigset, SIGTERM),-1, "sigaddset SIGTERM");
    SYSCALL(sigaddset(&sigset, SIGINT),-1, "sigaddset SIGINT");

    // assegno il set creato in precedenza al main, cosicché tutti i thread in esso creati erediteranno tale maschera
    DIVZERO(pthread_sigmask(SIG_SETMASK,&sigset,NULL),"pthread_sigmask in main");

    // il thread gestore dei segnali è di tipo 'detached' poiché non attendo la sua terminazione
    DIVZERO(pthread_attr_init(&attr),"pthread_attr_init in main")
    DIVZERO(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED),"pthread_attr_setdetachstate in main");
    DIVZERO(pthread_create(&sig_manager,&attr,&sig_manager_function,(void*)&sigset),"sig_manager pthread_create in main");

    /* ------- Parsing del file di configurazione ------- */

    int opt; /* opzione della riga di comando */

    if (argc < 2)
        usage(argv[0]);

    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
        switch (opt)
        {
            case 'f':
                conf_filepath = strdup(optarg);
                break;

            default:
                usage(argv[0]);
                break;
        }
    }

    /* ------- Inizializzazione della pipe  ------- */
    SYSCALL(pipe(answers),-1,"pipe in main");

    /* ------- Inizializzazione dei parametri di configurazione del server ------- */
    SYSCALL( conf = (config_t*) malloc(sizeof(config_t)), NULL, "malloc conf in main")
    parseConfigurationFile(conf,conf_filepath);

    //creo la directory (se essa non esiste) in cui salverò i file ricevuti dai client
    struct stat st;
    if (stat(conf->DirName, &st) == -1)
        mkdir(conf->DirName, 0777);

    // il numero di partizioni della hash table è pari al numero di thread nel pool
    int npartitions = conf->ThreadsInPool;// + (MAX_ICL_BUCKETS % conf->ThreadsInPool);
    assert(npartitions <= MAX_MTX_USR);

    /* ------- Inizializzazione delle mutex ------ */
    int m;
    for(m = 0; m<npartitions; m++)
        DIVZERO(pthread_mutex_init(&mtx_users[m],NULL),"pthread_mutex_init mtx_users in main");

    for(m = 0; m<MAX_MTX_REQ; m++)
        DIVZERO(pthread_mutex_init(&mtx_req[m],NULL),"pthread_mutex_init mtx_req in main");

    /* ------- Creazione hash table degli gli utenti registrati ------- */
    SYSCALL(users = icl_hash_create(MAX_ICL_BUCKETS,npartitions,NULL,NULL),
            NULL,"icl_hash_create in main");

    /* ------- Creazione del pool di thread ------ */
    if ((thpool = threadpool_create(conf->ThreadsInPool,MAX_QUEUE)) == NULL)
        ERRORE("threadpool_create in main");

    /* ------- Creazione socket di connessione con i client ------ */
    unlinkSocket();
    int sfd=0;
    struct sockaddr_un sa;

    //creo il descrittore per il socket
    SYSCALL( sfd = socket(AF_UNIX,SOCK_STREAM,0) , -1, "socket in main");

    //inizializzo la struttura sockaddr_un
    memset(&sa,'0', sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, conf->UnixPath, sizeof(sa.sun_path)-1);

    //con bind associo il socket all'indirizzo specificato nella struttura precedente
    SYSCALL( bind(sfd,(struct sockaddr *) &sa, sizeof(sa)) , -1, "bind in main");

    /* con listen specifico che il socket può accettare altre connessioni da parte di altri processi
     * fino a un numero massimo (specificato nel file di configurazione) */
    SYSCALL( listen(sfd,conf->MaxConnections) , -1 , "listen in main");

    /* ------- Creazione del thread listener ------- */
    DIVZERO(pthread_create(&listener,NULL,&listener_function,(void*)(unsigned long)sfd),"listener pthread_create in main");


    // attendo la terminazione del thread listener, per chiudere il server
    int status_listener;
    pthread_join(listener,(void*)&status_listener);

    /* ----- TERMINAZIONE SERVER ----- */
    close(sfd);
    pthread_attr_destroy(&attr);
    return 0;
}

/* IMPLEMENTAZIONE FUNZIONI DI UTILITÀ*/
static void usage(const char *progname)
{
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

static void unlinkSocket()  { unlink(conf->UnixPath); }

void cleanup()
{
    close(answers[0]);
    close(answers[1]);
    if(conf_filepath) free(conf_filepath);
    if(conf) conf_destroy(conf);
    if(thpool) threadpool_destroy(thpool,0);
    if(users) icl_hash_destroy(users,free,deleteQueue);
}

void* listener_function(void *connfd)
{
    //imposto il thread come cancellabile in qualsiasi momento
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    fd_set set,tmpset;
    long sfd = (unsigned long) connfd;
    int fdc;

    // azzero il master set e il set temporaneo usato nella select
    FD_ZERO(&set);
    FD_ZERO(&tmpset);

    // aggiungo FD del socket e della pipe al master set
    FD_SET(sfd, &set);
    FD_SET(answers[0], &set);

    // tengo traccia del FD con id più grande
    int fdmax = (sfd > answers[0]) ? (int)sfd : answers[0];

    while(1)
    {
        // copio il set nella variabile temporanea per la select
        tmpset = set;
        SYSCALL(select(fdmax+1, &tmpset, NULL, NULL, NULL),-1, "select in listener_function");

        // identifico il FD dal quale ho ricevuto una richiesta
        for(int fd=0; fd <= fdmax; fd++)
        {
            if (FD_ISSET(fd, &tmpset))
            {
                if (fd == sfd) // nuova richiesta di connessione
                {
                    SYSCALL(fdc = accept((int)sfd, (struct sockaddr*)NULL ,NULL),-1, "accept in listener_function");
                    FD_SET(fdc, &set);
                    if(fdc > fdmax)
                        fdmax = fdc;
                }
                else if(fd == answers[0]) // fine dell'esecuzione di una richiesta
                {
                    LOCK(mtx_pipe,"mtx_pipe in listener_function");
                    SYSCALL(read(fd,&fdc,sizeof(fdc)),-1,"read fdc in listener_function");
                    UNLOCK(mtx_pipe,"mtx_pipe in listener_function");

                    //il listener ricomincia ad ascoltare il client, la cui richiesta è stata soddisfatta
                    FD_SET(fdc, &set);

                    if(fdc > fdmax)
                        fdmax = fdc;
                    break;
                }
                else /* sock I/0 pronto */
                {
                    FD_CLR(fd,&set);

                    // controllo di non aver raggiunto il massimo numero di utenti connessi consentiti dal server
                    if(chattyStats.nonline >= conf->MaxConnections)
                    {
                        message_hdr_t hdr_reply;
                        setHeader(&hdr_reply, OP_FAIL, "server");
                        sendHeader(fd, &hdr_reply);
                    }
                    else
                    {
                        if ((threadpool_add(thpool, chooseRequest,(void*)(unsigned long) fd)) < 0)
                            ERRORE("threadpool_add in listener_function")
                    }
                }
            }
        }
    }
}

void* sig_manager_function(void* sigset)
{
    sigset_t *set = sigset;
    int sig;

    while(1)
    {
        DIVZERO(sigwait(set, &sig),"sigwait in sig_manager_function");

        //intercetto i segnali che mi interessano
        switch(sig)
        {
            case SIGUSR1: /* stampo le statistiche sul file indicato nella configurazione */
            {
                if(conf->StatFileName) //se l'opzione è vuota, ignoro il segnale
                {
                    FILE *stat_file;
                    stat_file = fopen(conf->StatFileName, "w+");
                    LOCK(mtx_stats, "mtxstats in sig_manager_function");
                    printStats(stat_file);
                    UNLOCK(mtx_stats, "mtxstats in sig_manager_function");
                    fclose(stat_file);
                }
                break;
            }
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
            {
                DIVZERO(pthread_cancel(listener),"pthread_cancel in sig_manager_function");
                pthread_exit(NULL);
            }
            default:
                break;
        }
    }
}

void chooseRequest(void* fd)
{
    int fdc = (int)((long)fd);
    int res;
    message_t msg;
    SYSCALL(res = readMsg(fdc,&msg),-1,"readMsg in chooseRequest")
    if(res == 0)
    {
        if (icl_hash_set_offline(users,fdc) == 0)
        {
            LOCK(mtx_stats, "mtx_stats in chooseRequest");
            chattyStats.nonline--;
            UNLOCK(mtx_stats, "mtx_stats in chooseRequest");
        }
        close(fdc);
    }
    else
    {
        //esegui richiesta
        switch (msg.hdr.op)
        {
            case REGISTER_OP:
                registerUser(fdc, msg.hdr.sender);
                break;

            case UNREGISTER_OP:
                unregisterUser(fdc, msg.data.hdr.receiver);
                break;

            case CONNECT_OP:
                connectUser(fdc, msg.hdr.sender);
                break;

            case DISCONNECT_OP: //non implementato nel client
                icl_hash_set_offline(users,fdc);
                break;

            case POSTTXT_OP:
                postText(fdc, msg);
                break;

            case POSTTXTALL_OP:
                postTextAll(fdc,msg);
                break;

            case GETFILE_OP:
                getFile(fdc,msg);
                break;

            case POSTFILE_OP:
                postFile(fdc, msg);
                break;

            case USRLIST_OP:
                usrList(fdc,msg.hdr.sender);
                break;

            case GETPREVMSGS_OP:
                getPrevMSGS(fdc,msg);
                break;

            default: //operazione non riconosciuta dal server
            {
                int mtxnum = fdc % MAX_MTX_REQ;
                message_hdr_t hdr_reply;
                setHeader(&hdr_reply, OP_FAIL, "server");
                LOCK(mtx_req[mtxnum],"mtx_req in chooseRequest");
                sendHeader(fdc,&hdr_reply);
                UNLOCK(mtx_req[mtxnum],"mtx_req in chooseRequest");
                break;
            }
        }

        //scrivo sulla pipe il fd, così da segnalare al listener l'esecuzione della richiesta
        LOCK(mtx_pipe,"mtx_pipe in chooseRequest");
        SYSCALL(write(answers[1],&fdc,sizeof(fdc)),-1,"write fdc in chooseRequest");
        UNLOCK(mtx_pipe,"mtx_pipe in chooseRequest");
    }
}

/* IMPLEMENTAZIONE OPERAZIONI DEL SERVER */

void registerUser(long fd, char* nickname)
{
    int partition;
    int mtxnum = (int)fd % MAX_MTX_REQ;
    message_t reply;
    int res;

    SYSCALL(partition = icl_hash_get_partition(users,nickname),-1,"icl_hash_get_partition in registerUser");
    LOCK(mtx_users[partition],"mtxusers registerUser");
    //SYSCALL(mtxnum = icl_hash_get_mtxnum(users,nickname,MAX_MTX_REQ),-1,"icl_hash_get_mtxnum in registerUser");

    if((res = icl_hash_isRegistered(users,nickname)) == 0) // nickname non registrato, lo resgistro
    {
        //lo aggiungo alla tabella hash
        SYSCALL(icl_hash_insert(users,nickname,(int)fd),NULL,"icl_hash_insert in registerUser");
        UNLOCK(mtx_users[partition],"mtxusers registerUser");

        //aggiorno le statistiche e ottengo la lista degli utenti online
        LOCK(mtx_stats,"mtxstats in registerUser");
        chattyStats.nusers++;
        chattyStats.nonline++;
        //int nonline = (int) ++chattyStats.nonline;
        UNLOCK(mtx_stats,"mtxstats in registerUser");

        usrList(fd,nickname);
    }
    else // utente già registrato o errore generico
    {
        UNLOCK(mtx_users[partition],"mtxusers registerUser");

        //aggiorno il numero di errori
        LOCK(mtx_stats,"mtxstats in registerUser");
        chattyStats.nerrors++;
        UNLOCK(mtx_stats,"mtxstats in registerUser");

        //risposta: nickname già utilizzato
        op_t op = (res == 1) ? OP_NICK_ALREADY : OP_FAIL;
        setHeader(&reply.hdr,op,"server");
        LOCK(mtx_req[mtxnum],"mtxreq registerUser");
        sendHeader(fd,&reply.hdr);
        UNLOCK(mtx_req[mtxnum],"mtxreq registerUser");
    }
}

void unregisterUser(long fd, char* nickname)
{
    int partition;
    int mtxnum = (int)fd % MAX_MTX_REQ;
    message_hdr_t reply;
    int res;

    SYSCALL(partition = icl_hash_get_partition(users,nickname),-1,"icl_hash_get_partition in unregisterUser");
    LOCK(mtx_users[partition],"mtxusers unregisterUser");

    if((res = icl_hash_isRegistered(users,nickname)) == 1) // nickname valido (infatti è registrato), lo deregistro
    {
        SYSCALL(icl_hash_delete(users,nickname,free,deleteQueue),-1,"icl_hash_delete in unregisterUser");
        UNLOCK(mtx_users[partition],"mtxusers unregisterUser");

        //aggiorno statistiche
        LOCK(mtx_stats,"mtxstats in unregisterUser");
        chattyStats.nonline--;
        chattyStats.nusers--;
        UNLOCK(mtx_stats,"mtxstats in unregisterUser");
        //risposta: OP_OK al sender
        setHeader(&reply,OP_OK,"server");
        LOCK(mtx_req[mtxnum],"mtxreq unregisterUser");
        sendHeader(fd,&reply);
        UNLOCK(mtx_req[mtxnum],"mtxreq unregisterUser");
    }
    else // utente NON registrato, errore
    {
        UNLOCK(mtx_users[partition],"mtxusers unregisterUser");

        //aggiorno il numero di errori
        LOCK(mtx_stats,"mtxstats in unregisterUser");
        chattyStats.nerrors++;
        UNLOCK(mtx_stats,"mtxstats in unregisterUser");

        //risposta: nickname inesistente nella tabella o errore generico

        op_t op = (res == 0) ? OP_NICK_UNKNOWN : OP_FAIL;
        setHeader(&reply,op,"server");
        LOCK(mtx_req[mtxnum],"mtxreq unregisterUser");
        sendHeader(fd,&reply);
        UNLOCK(mtx_req[mtxnum],"mtxreq unregisterUser");
    }
}

void connectUser(long fd, char* nickname)
{
    int partition;
    int mtxnum = (int)fd % MAX_MTX_REQ;
    message_t reply;
    int res;

    SYSCALL(partition = icl_hash_get_partition(users,nickname),-1,"icl_hash_get_partition in connectUser");
    LOCK(mtx_users[partition],"mtx_users connectUser");

    if((res = icl_hash_isRegistered(users,nickname)) == 1) //utente registrato, aggiorno
    {
        if(!icl_hash_isOnline(users,nickname)) //se l'utente NON è già online, aggiorno le statistiche
        {
            UNLOCK(mtx_users[partition],"mtx_users connectUser");
            LOCK(mtx_stats,"mtxstats in connectUser");
            chattyStats.nonline++;
            UNLOCK(mtx_stats,"mtxstats in connectUser");
            LOCK(mtx_users[partition],"mtx_users connectUser");
        }

        //se l'utente è già online aggiorno lo stato dell'utente che vuole connettersi
        SYSCALL(icl_hash_set_online(users,nickname,(int)fd),-1,"icl_hash_set_online in connectUser");
        UNLOCK(mtx_users[partition],"mtx_users connectUser");

        //mando lista utenti
        usrList(fd,nickname);
    }
    else //utente registrato, controllo il suo stato
    {
        UNLOCK(mtx_users[partition],"mtx_users connectUser");
        //aggiorno numero di errori
        LOCK(mtx_stats,"mtxstats in connectUser");
        chattyStats.nerrors++;
        UNLOCK(mtx_stats,"mtxstats in connectUser");

        //rispondo con: utente sconosciuto
        op_t op = (res == 0) ? OP_NICK_UNKNOWN : OP_FAIL;
        setHeader(&reply.hdr,op,"server");
        LOCK(mtx_req[mtxnum],"mtxreq connectUser");
        sendHeader(fd,&reply.hdr);
        UNLOCK(mtx_req[mtxnum],"mtxreq connectUser");
    }
}

void getFile(long fd, message_t msg)
{
    message_t reply;
    char* path, *filename;
    int mtxnum = (int)fd % MAX_MTX_REQ;

    SYSCALL(path = (char*) malloc(sizeof(char)*MAX_FILE_PATH),NULL,"malloc path in getFile");
    strncpy(path,conf->DirName,strlen(conf->DirName)+1);
    if ((filename = strrchr(msg.data.buf,'/')) == NULL) //file nel formato /.../<nome_file>
    {
        strncat(path,"/",1);
        SYSCALL(filename = strndup(msg.data.buf,strlen(msg.data.buf)+1),NULL,"strndup filename in getFile");
    }
    strncat(path,filename,strlen(filename)+1);

    FILE *f;
    if ((f = fopen(path,"r")) == NULL)
    {
        //file non esistente , o errore generico nell'apertura del file
        op_t op = (errno == EACCES) ? OP_NO_SUCH_FILE : OP_FAIL;
        setHeader(&reply.hdr,op,"server");
        LOCK(mtx_req[mtxnum],"mtx_req in getFile");
        sendHeader(fd,&reply.hdr);
        UNLOCK(mtx_req[mtxnum],"mtx_req in getFile");
    }
    else
    {
        struct stat st;
        SYSCALL(stat(path,&st),-1,"stat in getFile");
        long flen = st.st_size;

        char* buf;
        SYSCALL(buf = (char*)malloc(sizeof(char)*flen),NULL,"malloc buf in getFile");

        if ( fread(buf,(size_t)flen,1,f) != 1)
            ERRORE("fread in getFile");

        setHeader(&reply.hdr,OP_OK,"server");
        setData(&reply.data,"",buf,(unsigned int)flen);
        LOCK(mtx_req[mtxnum],"mtx_req in getFile");
        sendRequest(fd,&reply);
        UNLOCK(mtx_req[mtxnum],"mtx_req in getFile");

        LOCK(mtx_stats,"mtxstats in getFile");
        chattyStats.nfilenotdelivered--;
        chattyStats.nfiledelivered++;
        UNLOCK(mtx_stats,"mtxstats in getFile");

        free(buf);
    }

    if(f!=NULL) fclose(f);
    free(path);
    free(filename);
    free(msg.data.buf);
}

void postFile(long fd, message_t msg)
{
    int partition;
    int mtxnums, mtxnumr;
    message_t replyr;
    message_hdr_t replys;

    SYSCALL(partition = icl_hash_get_partition(users,msg.data.hdr.receiver),-1,"icl_hash_get_partition in postFile");
    mtxnums = (int) fd % MAX_MTX_REQ;

    LOCK(mtx_req[mtxnums],"mtx_req in postFile");
    message_data_t file;
    readData(fd,&file);
    UNLOCK(mtx_req[mtxnums],"mtx_req in postFile");
    int len = file.hdr.len;


    LOCK(mtx_users[partition],"mtx_users postFile");

    if(!icl_hash_isRegistered(users,msg.data.hdr.receiver)) //se destinatario non è registrato
    {
        UNLOCK(mtx_users[partition],"mtx_users postFile");
        //aggiorno numero di errori
        LOCK(mtx_stats,"mtxstats in postFile");
        chattyStats.nerrors++;
        UNLOCK(mtx_stats,"mtxstats in postFile");

        //non conoscono mittente e/o destinatario, quindi errore
        setHeader(&replys,OP_NICK_UNKNOWN,"server");
        LOCK(mtx_req[mtxnums],"mtxreq postFile");
        sendHeader(fd,&replys);
        UNLOCK(mtx_req[mtxnums],"mtxreq postFile");
    }
    else //destinatario registrato, preparo il file
    {
        UNLOCK(mtx_users[partition],"mtx_users postFile");

        if(len/1024 > conf->MaxFileSize || len <= 0) //lunghezza non consentita
        {
            LOCK(mtx_stats,"mtxstats in postFile");
            chattyStats.nerrors++;
            UNLOCK(mtx_stats,"mtxstats in postFile");

            op_t op = (len <= 0) ? OP_FAIL : OP_MSG_TOOLONG;
            setHeader(&replys,op,"server");

            LOCK(mtx_req[mtxnums],"mtx_req postFile");
            sendHeader(fd,&replys);
            UNLOCK(mtx_req[mtxnums],"mtx_req postFile");
        }
        else
        {
            LOCK(mtx_stats,"mtxstats in postFile");
            chattyStats.nfilenotdelivered++;
            UNLOCK(mtx_stats,"mtxstats in postFile");

            char* path;
            SYSCALL(path = (char*) malloc(sizeof(char)*MAX_FILE_PATH),NULL,"malloc path in postFile");
            strncpy(path,conf->DirName,strlen(conf->DirName)+1);
            char* filename;
            if ((filename = strrchr(msg.data.buf,'/')) == NULL) //file nel formato /.../<nome_file>
            {
                SYSCALL(filename = strndup(msg.data.buf,strlen(msg.data.buf)+1),NULL,"strndup filename in postFile");
            }
            else
            {
                filename++; //non considero lo slash '/'
                SYSCALL(filename = strndup(filename,strlen(filename)+1),NULL,"strndup filename in postFile");
            }

            strncat(path,"/",1);
            strncat(path,filename,strlen(filename)+1);

            FILE *f;
            SYSCALL(f = fopen(path,"w+"),NULL,"fopen in postFile");
            if ( fwrite(file.buf,file.hdr.len,1,f) != 1)
                ERRORE("fwrite in postFile");
            fclose(f);

            //aggiungo il messaggio nella history del destinatario
            LOCK(mtx_users[partition],"mtx_users postFile");

            setHeader(&replyr.hdr,FILE_MESSAGE,msg.hdr.sender);
            setData(&replyr.data,"",filename,(unsigned int) strlen(filename)+1);
            SYSCALL(icl_hash_addMessage(users,msg.data.hdr.receiver,replyr),-1,"icl_addMessage in postFile");
            int fdr = (icl_hash_find(users, msg.data.hdr.receiver))->fd;
            if(icl_hash_isOnline(users,msg.data.hdr.receiver)) //receiver online, mando il file "immediatamente"
            {
                //SYSCALL(mtxnumr = icl_hash_get_mtxnum(users,msg.data.hdr.receiver,MAX_MTX_REQ),-1,"icl_hash_get_mtxnumr in postFile");
                mtxnumr = fdr % MAX_MTX_REQ;
                UNLOCK(mtx_users[partition],"mtx_users in postFile");
                LOCK(mtx_req[mtxnumr],"mtx_req postFile");
                sendRequest(fdr,&replyr);
                UNLOCK(mtx_req[mtxnumr],"mtx_req postFile");

                //aggiorno le statistiche
                LOCK(mtx_stats,"mtxstats in postFile");
                chattyStats.nfilenotdelivered--;
                chattyStats.nfiledelivered++;
                UNLOCK(mtx_stats,"mtxstats in postFile");
            }
            else
            {
                UNLOCK(mtx_users[partition], "mtx_users in postFile");
            }

            free(path);
            free(msg.data.buf);
            //free(filename);

            //mando OP_OK al mittente
            setHeader(&replys,OP_OK,msg.hdr.sender);
            LOCK(mtx_req[mtxnums],"mtx_req postFile");
            sendHeader(fd,&replys);
            UNLOCK(mtx_req[mtxnums],"mtx_req postFile");
        }
    }
    free(file.buf);
    //free(msg.data.buf);
}

void postText(long fd, message_t msg)
{
    message_t replyr;
    message_hdr_t replys;
    int partition;
    int mtxnums = (int) fd % MAX_MTX_REQ;
    int mtxnumr;
    int reg;

    SYSCALL(partition = icl_hash_get_partition(users,msg.data.hdr.receiver),-1,"icl_hash_get_partition in postText");
    LOCK(mtx_users[partition],"mtx_users postText");

    //se destinatario non è registrato o messaggio troppo lungo, fallisco
    if((reg = icl_hash_isRegistered(users,msg.data.hdr.receiver)) == 0 || msg.data.hdr.len > conf->MaxMsgSize)
    {
        UNLOCK(mtx_users[partition],"mtx_users postText");
        //aggiorno numero di errori
        LOCK(mtx_stats,"mtxstats in postText");
        chattyStats.nerrors++;
        UNLOCK(mtx_stats,"mtxstats in postText");

        op_t op = (reg == 0) ? OP_NICK_UNKNOWN : OP_MSG_TOOLONG;
        setHeader(&replys,op,"server");

        LOCK(mtx_req[mtxnums],"mtx_req postText");
        sendHeader(fd,&replys);
        UNLOCK(mtx_req[mtxnums],"mtx_req postText");
    }
    else //destinatario registrato, preparo il messaggio testuale
    {
        UNLOCK(mtx_users[partition],"mtx_users postText");

        LOCK(mtx_stats,"mtxstats in postText");
        chattyStats.nnotdelivered++;
        UNLOCK(mtx_stats,"mtxstats in postText");

        LOCK(mtx_users[partition],"mtx_users postText");

        //aggiungo il messaggio nella history del destinatario
        setHeader(&replyr.hdr,TXT_MESSAGE,msg.hdr.sender);
        setData(&replyr.data,"",msg.data.buf,msg.data.hdr.len);
        SYSCALL(icl_hash_addMessage(users,msg.data.hdr.receiver,replyr),-1,"icl_addMessage in postText");

        int fdr = (icl_hash_find(users, msg.data.hdr.receiver))->fd;
        if(icl_hash_isOnline(users,msg.data.hdr.receiver)) //receiver online, mando il messaggio "immediatamente"
        {
            //SYSCALL(mtxnumr = icl_hash_get_mtxnum(users,msg.data.hdr.receiver,MAX_MTX_REQ),-1,"icl_hash_get_mtxnumr in postText");
            mtxnumr = fdr % MAX_MTX_REQ;
            UNLOCK(mtx_users[partition],"mtx_users in postText");
            LOCK(mtx_req[mtxnumr],"mtx_req postText");
            sendRequest(fdr,&replyr);
            UNLOCK(mtx_req[mtxnumr],"mtx_req postText");

            //aggiorno le statistiche
            LOCK(mtx_stats,"mtxstats in postText");
            chattyStats.nnotdelivered--;
            chattyStats.ndelivered++;
            UNLOCK(mtx_stats,"mtxstats in postText");
        }
        else
        {
            UNLOCK(mtx_users[partition], "mtx_users in postText");
        }

        //mando OP_OK al mittente
        setHeader(&replys,OP_OK,msg.hdr.sender);
        LOCK(mtx_req[mtxnums],"mtx_req postText");
        sendHeader(fd,&replys);
        UNLOCK(mtx_req[mtxnums],"mtx_req postText");
    }
    //free(msg.data.buf);
}

void postTextAll(long fd, message_t msg)
{
    message_t replyr;
    message_hdr_t replys;
    int partition;
    int mtxnums = (int) fd % MAX_MTX_REQ;;
    int mtxnumr;

    //se destinatario non è registrato o messaggio troppo lungo, fallisco
    if(msg.data.hdr.len > conf->MaxMsgSize)
    {
        //aggiorno numero di errori
        LOCK(mtx_stats,"mtxstats in postTextAll");
        chattyStats.nerrors++;
        UNLOCK(mtx_stats,"mtxstats in postTextAll");

        setHeader(&replys,OP_MSG_TOOLONG,"server");

        LOCK(mtx_req[mtxnums],"mtx_req postTextAll");
        sendHeader(fd,&replys);
        UNLOCK(mtx_req[mtxnums],"mtx_req postTextAll");
    }
    else
    {
        icl_entry_t *bkt, *curr;
        int i,err=0;
        for(i=0; i<users->nbuckets; i++)
        {
            bkt = users->buckets[i];
            for(curr = bkt; curr!=NULL; curr=curr->next)
            {
                if(strcmp(msg.hdr.sender,curr->key) != 0) //salto l'invio al mittente stesso
                {
                    LOCK(mtx_stats,"mtxstats in postTextAll");
                    chattyStats.nnotdelivered++;
                    UNLOCK(mtx_stats,"mtxstats in postTextAll");

                    //preparo il messaggio da mandare/salvare nella coda

                    char* buf;
                    //SYSCALL(buf = (char*) malloc(sizeof(char)*(msg.data.hdr.len)+1),NULL,"malloc buf in postText");
                    buf = strndup(msg.data.buf,msg.data.hdr.len);

                    setHeader(&replyr.hdr,TXT_MESSAGE,msg.hdr.sender);
                    setData(&replyr.data,"",buf,msg.data.hdr.len);
                    SYSCALL(partition = icl_hash_get_partition(users,curr->key),-1,"icl_hash_get_partition in postTextAll");

                    LOCK(mtx_users[partition],"mtxusers in postTextAll");

                    //salvo il messaggio nella coda di curr

                    //if(icl_hash_addMessage(users,curr->key,replyr) == -1)
                    if (push(curr->queue,replyr) == -1)
                    {
                        UNLOCK(mtx_users[partition],"mtxusers in postTextAll");
                        LOCK(mtx_stats,"mtxstats in postTextAll");
                        chattyStats.nerrors++;
                        UNLOCK(mtx_stats,"mtxstats in postTextAll");
                        err = 1;
                        LOCK(mtx_users[partition],"mtxusers in postTextAll");
                    }

                    if(curr->online == 1) //utente online, mando subito il messaggio
                    {
                        mtxnumr = curr->fd % MAX_MTX_REQ;
                        UNLOCK(mtx_users[partition],"mtxusers in postTextAll");

                        LOCK(mtx_req[mtxnumr],"mtx_req in postTextAll");
                        sendRequest(curr->fd,&replyr);
                        UNLOCK(mtx_req[mtxnumr],"mtx_req in postTextAll");

                        LOCK(mtx_stats,"mtxstats in postTextAll");
                        chattyStats.nnotdelivered--;
                        chattyStats.ndelivered++;
                        UNLOCK(mtx_stats,"mtxstats in postTextAll");

                        LOCK(mtx_users[partition],"mtxusers in postTextAll");
                    }
                    UNLOCK(mtx_users[partition],"mtxusers in postTextAll");
                }
            }
        }
        op_t op = (err) ? OP_FAIL : OP_OK;
        setHeader(&replys,op,"server");
        LOCK(mtx_req[mtxnums],"mtx_req postTextAll");
        sendHeader(fd,&replys);
        UNLOCK(mtx_req[mtxnums],"mtx_req postTextAll");
    }
    free(msg.data.buf);
}

void usrList(long fd, char* sender)
{
    message_t reply;
    int partition;
    int mtxnum = (int) fd % MAX_MTX_REQ;
    SYSCALL(partition = icl_hash_get_partition(users,sender),-1,"icl_hash_get_partition in usrList");

    //ottengo il numero di persone attualmente online
    LOCK(mtx_stats,"mtxstats in usrList");
    int nonline = (int) chattyStats.nonline;
    UNLOCK(mtx_stats,"mtxstats in usrList");

    //ottengo la lista delle persone online
    LOCK(mtx_users[partition],"mtxusers usrList");
    char* usrlist = NULL;
    usrlist = icl_hash_get_onlineusers(users,conf->MaxConnections);
    UNLOCK(mtx_users[partition],"mtxusers usrList");

    //rispondo con OP_OK e invio la lista degli utenti
    setHeader(&reply.hdr,OP_OK,"server");
    setData(&reply.data,"",usrlist,(unsigned int) nonline*(MAX_NAME_LENGTH+1));
    LOCK(mtx_req[mtxnum],"mtxreq usrList");
    sendRequest(fd,&reply);
    UNLOCK(mtx_req[mtxnum],"mtxreq usrList");

    free(usrlist);
}

void getPrevMSGS(long fd, message_t msg)
{
    message_t reply;
    int partition;
    int mtxnum = (int) fd % MAX_MTX_REQ;

    SYSCALL(partition = icl_hash_get_partition(users,msg.hdr.sender),-1,"icl_hash_get_partition in getPrevMSGS");

    LOCK(mtx_users[partition],"mtx_req in getPrevMSGS");
    queue_t *queue = (icl_hash_find(users,msg.hdr.sender))->queue;
    size_t nmsgs = queueLength(queue);
    UNLOCK(mtx_users[partition],"mtx_req in getPrevMSGS");

    //mando prima il numero di messaggi che il client dovrà leggere
    setHeader(&reply.hdr,OP_OK,"server");
    setData(&reply.data,"",(const char*)&nmsgs, (unsigned int) sizeof(size_t*));


    LOCK(mtx_req[mtxnum],"mtx_num in getPrevMSGS");
    sendRequest(fd,&reply);
    UNLOCK(mtx_req[mtxnum],"mtx_num in getPrevMSGS");

    //estraggo i messaggi dalla coda e li mando al client
    LOCK(mtx_users[partition],"mtx_req in getPrevMSGS");
    node_t *curr = queue->head;

    while(curr!=NULL)
    {
        reply = curr->msg;
        UNLOCK(mtx_users[partition],"mtx_req in getPrevMSGS");
        LOCK(mtx_req[mtxnum],"mtx_num in getPrevMSGS");
        sendRequest(fd,&reply);
        //free(reply.data.buf);
        UNLOCK(mtx_req[mtxnum],"mtx_num in getPrevMSGS");

        LOCK(mtx_stats,"mtx_stats in getPrevMSGS");
        if(reply.hdr.op == FILE_MESSAGE)
        {
            chattyStats.nfiledelivered++;
            chattyStats.nfilenotdelivered--;
        }
        else
        {
            chattyStats.ndelivered++;
            chattyStats.nnotdelivered--;
        }
        UNLOCK(mtx_stats,"mtx_stats in getPrevMSGS");
        LOCK(mtx_users[partition],"mtx_num in getPrevMSGS");
        curr = curr->next;
    }
    UNLOCK(mtx_users[partition],"mtx_num in getPrevMSGS");
}