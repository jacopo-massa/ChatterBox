/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file client.c
 * @brief Semplice client di test. 
 *
 *
 */
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


#include <connections.h>
#include <ops.h>

// tipo di operazione (usata internamente)
typedef struct {
    char  *sname;   // nickname del sender
    char  *rname;   // nickname o groupname del receiver 
    op_t   op;      // tipo di operazione (se OP_END e' una operazione interna)
    char  *msg;     // messaggio testuale o nome del file
    long   size;    // lunghezza del messaggio
    long   n;       // usato per -R -r
} operation_t;

/* -------------------- globali -------------------------- */
// array di messaggi ricevuti ma non ancora gestiti
static message_t  *MSGS = NULL;
static const int   msgbatch = 100;
static size_t      msgcur=0;
static size_t      msglen=0;     
/* ------------------------------------------------------- */

// usage function
static void use(const char * filename) {
    fprintf(stderr, 
	    "use:\n"
	    " %s -l unix_socket_path -k nick -c nick -[gad] group -t milli -S msg:to -s file:to -R n -h\n"
	    "  -l specifica il socket dove il server e' in ascolto\n"
	    "  -k specifica il nickname del client\n"
	    "  -c specifica il nickname che deve essere creato\n"
	    "  -C chiede che il nickname venga deregistrato\n"
	    "  -g specifica il groupname che deve essere creato\n"
	    "  -a aggiunge 'nick' al gruppo 'group'\n"
	    "  -d rimuove  'nick' dal gruppo 'group'\n"
	    "  -L richiede la lista degli utenti online\n"
	    "  -p richiede di recuperare la history dei messaggi\n"
	    "  -t specifica i millisecondi 'milli' che intercorrono tra la gestione di due comandi consecutivi\n"
	    "  -S spedisce il messaggio 'msg' al destinatario 'to' che puo' essere un nickname o groupname\n"
	    "  -s come l'opzione -S ma permette di spedire files\n"
	    "  -R riceve un messaggio da un nickname o groupname, se viene ricevuto un identificatore di file\n"
	    "     il file viene scaricato dal server. In base al valore di n il comportamento e' diverso, se:\n"
	    "      n > 0 : aspetta di ricevere 'n' messaggi e poi passa al comando successivo (se c'e')\n"
	    "      n <= 0: aspetta di ricevere uno o piu' messaggi indefinitamente (eventuali comandi che \n"
	    "             seguono non verranno eseguiti)\n",
	    filename);
}

// legge un messaggio (testuale o file) e lo memorizza in MSGS (array globale)
static int readMessage(int connfd, message_hdr_t *hdr) {
    
    if (readData(connfd, &MSGS[msgcur].data) <= 0) {
	perror("reading data");
	return -1; 
    }

    // NOTA: la gestione di MSGS e' molto brutale: non si libera mai memoria
    
    MSGS[msgcur].hdr = *hdr;
    msgcur++;
    if (msgcur>=msglen) {  
	msglen += msgbatch;
	MSGS= realloc(MSGS, msglen);
	if (MSGS == NULL) {
	    perror("realloc");
	    fprintf(stderr, "ERRORE: Out of memory\n");
	    exit(EXIT_FAILURE);
	}
	for(size_t i=msgcur;i<msglen;++i) MSGS[i].data.buf=NULL;
    }	    
    return 1;
}

// effettua la richiesta di download di un file
static int downloadFile(int connfd, char *filename, char *sender) {
    // mando la richiesta di download
    message_t msg;
    setHeader(&msg.hdr, GETFILE_OP, sender);
    setData(&msg.data, "", filename, strlen(filename)+1);
    if (sendRequest(connfd, &msg) == -1)  return -1;
    for( ; ;) {
	// aspetto di ricevere la risposta alla richiesta
	if (readHeader(connfd, &msg.hdr) <= 0) return -1;

	// differenti tipi di risposta che posso ricevere
	switch(msg.hdr.op) {
	case OP_OK: {
	    if (readData(connfd, &msg.data) <= 0) return -1;
	    return 0;
	} break;
	case TXT_MESSAGE:
	case FILE_MESSAGE: {  	    
	    /* Non ho ricevuto la risposta ma messaggi da altri client, 
	     * li conservo in MSGS per gestirli in seguito.
	     */
	    if (readMessage(connfd, &msg.hdr)<=0) return -1;
	} break;
	default: {
	    fprintf(stderr, "ERRORE: ricevuto messaggio non valido\n");
	    return -1;
	}
	}
    }
    return -1;
}

// gestisce operazioni di tipo richiesta-risposta
static int execute_requestreply(int connfd, operation_t *o) {
    char *sname = o->sname;
    char *rname = o->rname?o->rname:"";
    op_t op     = o->op;
    message_t msg;
    char  *mappedfile = NULL;
    
    //setData(&msg.data, "", NULL, 0);
    setData(&msg.data, rname, NULL, 0);
    setHeader(&msg.hdr, op, sname);
    if (op == POSTTXT_OP || op == POSTTXTALL_OP || op == POSTFILE_OP) {
	if (o->size == 0) {
	    fprintf(stderr, "ERRORE: size non valida per l'operazione di POST\n");
	    return -1;
	}	    
	if (op == POSTFILE_OP) {
	    int fd = open(o->msg, O_RDONLY);
	    if (fd<0) {
		perror("open");
		fprintf(stderr, "ERRORE: aprendo il file %s\n", o->msg);
		close(fd);
		return -1;
	    }
	    // mappiamo il file da spedire in memoria
	    mappedfile = mmap(NULL, o->size, PROT_READ,MAP_PRIVATE, fd, 0);
	    if (mappedfile == MAP_FAILED) {
		perror("mmap");
		fprintf(stderr, "ERRORE: mappando il file %s in memoria\n", o->msg);
		close(fd);
		return -1;
	    }
	    close(fd);
	    setData(&msg.data, rname, o->msg, strlen(o->msg)+1); // invio il nome del file
	} else 
	    setData(&msg.data, rname, o->msg, o->size);	    
    } 
    
    // spedizione effettiva
    if (sendRequest(connfd, &msg) == -1) {
	perror("request");
	return -1;
    }
    if (mappedfile) { // devo inviare il file
	message_data_t data;
	setData(&data, "", mappedfile, o->size);
	if (sendData(connfd, &data) == -1) { // invio il contenuto del file
	    perror("sending data");
	    fprintf(stderr, "ERRORE: spedendo il file %s\n", o->msg);
	    munmap(mappedfile, o->size);
	    return -1;
	}
	munmap(mappedfile, o->size);
    } else if (msg.data.buf) free(msg.data.buf);

    // devo ricevere l'ack
    int ackok = 0;
    while(!ackok) {
	// aspetto di ricevere la risposta alla richiesta
	if (readHeader(connfd, &msg.hdr) <= 0) {
	    perror("reply header");
	    return -1;
	}
	
	// differenti tipi di risposta che posso ricevere
	switch(msg.hdr.op) {
	case OP_OK:  ackok = 1;     break;
	case TXT_MESSAGE:
	case FILE_MESSAGE: {  	    
	    /* Non ho ricevuto la risposta ma messaggi da altri client, 
	     * li conservo in MSGS per gestirli in seguito.
	     */
	    if (readMessage(connfd, &msg.hdr)<=0) return -1;
	} break;
	case OP_NICK_ALREADY:
	case OP_NICK_UNKNOWN:
	case OP_MSG_TOOLONG:
	case OP_FAIL: {
	    if (msg.data.buf) fprintf(stderr, "Operazione %d FALLITA: %s\n", op, msg.data.buf);
	    else  	      fprintf(stderr, "Operazione %d FALLITA\n", op);
	    return -msg.hdr.op; // codice di errore ritornato
	} break;
	default: {
	    fprintf(stderr, "ERRORE: risposta non valida\n");
	    return -1;
	}
	}
    }
    // Ho ricevuto l'ack dal server, ora sulla base dell'operazione che avevo 
    // richiesto devo ...
    switch(op) {
    case REGISTER_OP:
    case CONNECT_OP:
    case USRLIST_OP: {  // ... ricevere la lista degli utenti
	if (readData(connfd, &msg.data) <= 0) {
	    perror("reply data");
	    return -1; 
	}	
	int nusers = msg.data.hdr.len / (MAX_NAME_LENGTH+1);
	assert(nusers > 0);
	printf("Lista utenti online:\n");
	for(int i=0,p=0;i<nusers; ++i, p+=(MAX_NAME_LENGTH+1)) {
	    printf(" %s\n", &msg.data.buf[p]);
	}
    } break;
    case GETPREVMSGS_OP: { // ... ricevere la lista dei vecchi messaggi
	if (readData(connfd, &msg.data) <= 0) {
	    perror("reply data");
	    return -1; 
	}	
	// numero di messaggi che devo ricevere
	size_t nmsgs = *(size_t*)(msg.data.buf); 
	char *FILENAMES[nmsgs]; // NOTA: si suppone che nmsgs non sia molto grande
	size_t nfiles=0;
	for(size_t i=0;i<nmsgs;++i) {
	    message_t pmsg;
	    // leggo l'intero messaggio
	    if (readMsg(connfd, &pmsg) <= 0) {
		perror("reply data");
		return -1; 
	    }	
	    if (pmsg.hdr.op == FILE_MESSAGE) {
		FILENAMES[nfiles] = strdup(pmsg.data.buf);
		nfiles++;
		printf("[%s vuole inviare il file '%s']\n", pmsg.hdr.sender, pmsg.data.buf);
	    } else 
		printf("[%s:] %s\n", pmsg.hdr.sender, (char*)pmsg.data.buf);
	}	    

	// scarico i file che ho ricevuto
	for(size_t i=0;i<nfiles;++i) {
	    if (downloadFile(connfd, FILENAMES[i], sname) == -1) {
		fprintf(stderr, "ERRORE: cercando di scaricare il file %s\n", FILENAMES[i]);
		return -1;
	    }
	    printf("[Il file '%s' e' stato scaricato correttamente]\n",FILENAMES[i]);
	}
    } break;
    case POSTTXT_OP:
    case POSTTXTALL_OP:
    case POSTFILE_OP:
    case DISCONNECT_OP:
    case UNREGISTER_OP: 
    case CREATEGROUP_OP: 
    case ADDGROUP_OP:
    case DELGROUP_OP: break;  // ... fare nulla
    default: {
	fprintf(stderr, "ERRORE: messaggio non valido\n");
	return -1;
    }
    }
    return 0;   
}

// gestisce operazioni di tipo richiesta-risposta
static int execute_receive(int connfd, operation_t *o) {
    char *sname = o->sname;
    size_t m    = (size_t)-1;
    
    if (o->n>0) m = (ssize_t)o->n;
    
    size_t c=0;
    for(size_t i=0; i<msgcur;++i) {
	if (MSGS[i].data.buf != NULL) {
	    if (MSGS[i].hdr.op == FILE_MESSAGE) {
		char *filename = MSGS[i].data.buf;

		printf("[%s vuole inviare il file '%s']\n", MSGS[i].hdr.sender, filename);

		if (downloadFile(connfd, filename, sname) == -1) {
		    fprintf(stderr, "ERRORE: cercando di scaricare il file %s\n", filename);
		    return -1;
		}
		printf("[Il file '%s' e' stato scaricato correttamente]\n",filename);
	    } else 
		printf("[%s:] %s\n", MSGS[i].hdr.sender, (char*)MSGS[i].data.buf);

	    if (++c == m) break;
	}
    }
    for(size_t i=c; i<m; ++i) {
	message_t msg;
	// leggo header e data
	if (readMsg(connfd, &msg) == -1) {
	    perror("reply data");
	    return -1; 
	}	
	switch(msg.hdr.op) {
	case TXT_MESSAGE: {
	    printf("[%s:] %s\n", msg.hdr.sender, (char*)msg.data.buf);
	} break;
	case FILE_MESSAGE: {
	    char *filename = strdup(msg.data.buf);
	    free(msg.data.buf);
	    printf("[%s vuole inviare il file '%s']\n", msg.hdr.sender, filename);

	    if (downloadFile(connfd, filename, sname) == -1) {
		fprintf(stderr, "ERRORE: cercando di scaricare il file %s\n", filename);
		return -1;
	    }
	    printf("[Il file '%s' e' stato scaricato correttamente]\n",filename);
	} break;
	default: {
	    fprintf(stderr, "ERRORE: ricevuto messaggio non valido\n");
	    return -1;
	}
	}	    
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const char optstring[] = "l:k:c:C:g:a:d:t:S:s:R:pLh";
    int optc;
    char *spath = NULL, *nick = NULL;
    operation_t *ops = NULL;
    long msleep=0;

    if (argc <= 4) {
	use(argv[0]);
	return -1;
    }

    // al piu' ci saranno argc-3 comandi da eseguire
    ops = (operation_t*)malloc(sizeof(operation_t)*(argc-3));
    if (!ops) {
	perror("malloc");
	return -1;
    }
    int k=0, nickneeded=0, coption=0;
    // parse command line options
    while ((optc = getopt(argc, argv,optstring)) != -1) {
 	switch (optc) {
        case 'l': spath=optarg;                   break;
	case 't': msleep= strtol(optarg,NULL,10); break;
	case 'k': {
	    nick = strdup(optarg);
	    if (strlen(nick)>MAX_NAME_LENGTH) {
		fprintf(stderr, "ERRORE: Nickname troppo lungo\n");
		return -1;
	    }
	    ops[k].sname = nick;
	    ops[k].rname = NULL;
	    ops[k].op    = CONNECT_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
        case 'c': {
	    coption++;   // -c puo' comparire solo 1 volta
	    ops[k].sname = strdup(optarg);
	    ops[k].rname = NULL;
	    ops[k].op    = REGISTER_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
        case 'C': {
	    nickneeded = 1;
	    ops[k].sname = nick;
	    ops[k].rname = strdup(optarg);
	    ops[k].op    = UNREGISTER_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
        case 'g': {
	    nickneeded = 1;
	    ops[k].sname = nick;
	    ops[k].rname = strdup(optarg);
	    ops[k].op    = CREATEGROUP_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
        case 'a': {
	    nickneeded = 1;
	    ops[k].sname = nick;
	    ops[k].rname = strdup(optarg);
	    ops[k].op    = ADDGROUP_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
        case 'd': {
	    nickneeded = 1;
	    ops[k].sname = nick;
	    ops[k].rname = strdup(optarg);
	    ops[k].op    = DELGROUP_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
	case 'L': {
	    nickneeded = 1;
	    ops[k].sname = nick;
	    ops[k].rname = NULL;
	    ops[k].op    = USRLIST_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
	case 'p': {
	    nickneeded = 1;
	    ops[k].sname = nick;
	    ops[k].rname = NULL;
	    ops[k].op    = GETPREVMSGS_OP;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
	case 'S': {
	    nickneeded = 1;
	    char *arg = strdup(optarg);
	    char *p;
	    p = strchr(arg, ':');
	    if (!p) {
		use(argv[0]);
		return -1;
	    }
	    *p++ = '\0';	

	    if (arg[0] == '\0') {
		fprintf(stderr, "ERRORE: messaggio vuoto\n");
		return -1;
	    }

	    ops[k].sname = nick;    
	    ops[k].rname = strlen(p)?p:NULL;
	    ops[k].op    = strlen(p)?POSTTXT_OP:POSTTXTALL_OP;
	    ops[k].msg   = arg;
	    ops[k].size  = strlen(arg)+1;

	    ++k;
	} break;
	case 's': {
	    nickneeded = 1;
	    char *arg = strdup(optarg);
	    char *p;
	    p = strchr(arg, ':');
	    if (!p) {
		use(argv[0]);
		return -1;
	    }
	    *p++ = '\0';
	    if (strlen(p)==0) {
		fprintf(stderr, "ERRORE: nell'opzione -s e' necessario definire il destinatario\n");
		use(argv[0]);
		return -1;
	    }
	    ops[k].sname = nick;
	    ops[k].rname = p;
	    ops[k].op    = POSTFILE_OP;
	    
	    // controllo che il file esista
	    struct stat st;
	    if (stat(arg, &st)==-1) {
		perror("stat");
		fprintf(stderr, "ERRORE: nella stat del file %s\n", arg);
		use(argv[0]);
		return -1;
	    }
	    if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "ERRORE: il file %s non e' un file regolare\n", arg);
		use(argv[0]);
		return -1;
	    }
	    ops[k].msg  = arg;        // nome del file
	    ops[k].size = st.st_size; // size del file 
	    ++k;
	} break;
	case 'R': {
	    nickneeded = 1;
	    ops[k].op    = OP_END; // operazione interna non invio nessuna richiesta al server
	    ops[k].n     = strtol(optarg,NULL,10);
	    ops[k].sname = nick;
	    ops[k].rname = NULL;
	    ops[k].msg   = NULL;
	    ops[k].size  = 0;
	    ++k;
	} break;
        case 'h':
	default: {
	    use(argv[0]);
	    return -1;
	}
	}
    }
    // verifichiamo alcune opzioni
    if (spath==NULL) {
	fprintf(stderr, "ERRORE: L'opzione -l deve essere presente\n\n");
	return -1;
    }
    if (nick == NULL && nickneeded) {
	fprintf(stderr, "ERRORE: L'opzione -k non e' stata specificata\n\n");
	return -1;
    }
    // qualora -k non venisse prima delle altre opzioni
    if (nickneeded) {
	for(int i=0;i<k;++i) 
	    if (ops[i].op != REGISTER_OP) ops[i].sname = nick;
    }
    // vincolo su -c
    if (coption>1) {
	fprintf(stderr, "ERRORE: L'opzione -c puo' comparire una sola volta\n\n");
	return -1;
    }

    int connfd;
    // faccio 10 tentativi aspettando 1 secondo tra due tentativi
    if ((connfd=openConnection(spath, 10, 1))<0) {
	fprintf(stderr, "ERRORE: riprovo a riconnettermi...\n");
	return -1;
    }

    // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket chiuso
    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    if ( (sigaction(SIGPIPE,&s,NULL) ) == -1 ) {   
	perror("sigaction");
	return -1;
    } 
    struct timespec req = { msleep/1000, (msleep%1000)*1000000L };  

    MSGS = malloc(msgbatch*sizeof(message_t));
    if (!MSGS) {
	perror("malloc");
	fprintf(stderr, "ERRORE: Out of memory\n");
	return -1;
    }
    msglen = msgbatch;
  
    int r=0;
    for(int i=0;i<k;++i) {
	if (ops[i].op != OP_END) 
	    r = execute_requestreply(connfd, &ops[i]);
	else 
	    r = execute_receive(connfd, &ops[i]);
	if (r == 0)  printf("Operazione %d eseguita con successo!\n", i);
	else break;  // non appena una operazione fallisce esco
	
	// tra una operazione e l'altra devo aspettare msleep millisecondi
	if (msleep>0) nanosleep(&req, (struct timespec *)NULL);
    }
    // la disconnessione del client non avviene in modo esplicito
    // (con la DISCONNECT_OP), ma in modo implicito chiudendo il socket.
    close(connfd);
    if (ops) free(ops);
    if (MSGS) free(MSGS);
    return r;
}

