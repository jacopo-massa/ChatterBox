/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file utility.h
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Funzioni e macro utili negli altri files del progetto
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

/**
 * @def ERRORE(m)
 * Stampa su standard output il messaggio \a m e termina
 */
#define ERRORE(m) {perror(m); exit(EXIT_FAILURE);}

/**
 * @def SYSCALL(s,g,m)
 * Confronta il valore di ritorno di uno statement con \a g, e stampa il messaggio \a m in caso negativo
 */
#define SYSCALL(s,g,m) if((s) == g) ERRORE(m)

/**
 * @def DIVZERO(s,m)
 * Confronta il valore di ritorno di uno statement con \a 0, e stampa il messaggio \a m in caso negativo
 */
#define DIVZERO(s,m) if((s) != 0) ERRORE(m)

/**
 * @def EQZERO(n)
 * Confronta il valore di ritorno di uno statement con \a 0, ritornando \a 0 in caso positivo
 */
#define EQZERO(n) if(n==0) return 0;

/**
 * @def LOCK(l,m)
 * Esegue una lock sulla mutex \a l, e stampa il messaggio \a m in caso di errore
 */
#define LOCK(l,m) DIVZERO(pthread_mutex_lock(&l),strcat("lock ",m))

/**
 * @def UNLOCK(l,m)
 * Esegue una unlock sulla mutex \a l, e stampa il messaggio \a m in caso di errore
 */
#define UNLOCK(l,m) DIVZERO(pthread_mutex_unlock(&l),strcat("unlock ",m))


/**
 * @typedef config_t
 * @brief Ridefinizione della struttura config_s
 *
 * @struct config_s
 * @brief Contiene i parametri di configurazione del server
 *
 * @param[in] UnixPath          path utilizzato per la creazione del socket AF_UNIX
 * @param[in] MaxConnections    numero massimo di connessioni pendenti
 * @param[in] ThreadsInPool     numero di thread nel pool
 * @param[in] MaxMsgSize        dimensione massima di un messaggio testuale (numero di caratteri)
 * @param[in] MaxFileSize       dimensione massima di un file accettato dal server (kilobytes)
 * @param[in] MaxHistMsgs       numero massimo di messaggi che il server ricorda per ogni client
 * @param[in] DirName           directory dove memorizzare i files da inviare agli utenti
 * @param[in] StatFileName      file nel quale verranno scritte le statistiche del server
 */
typedef struct config_s
{
    char* UnixPath;
    int MaxConnections;
    int ThreadsInPool;
    int MaxMsgSize;
    int MaxFileSize;
    int MaxHistMsgs;
    char* DirName;
    char* StatFileName;

}config_t;


/**
 * @function parseConfigurationFile
 * @brief Legge i parametri di configurazione da file inizializzando la struttura passata come argomento
 *
 * @param conf struttura da inizializzare
 * @param filepath path del file di configurazione
 */
void parseConfigurationFile(config_t *conf, char* filepath);

/**
 * @function conf_destroy
 * @brief Libera la memoria allocata per la struttura di configurazione
 *
 * @param conf puntatore alla struttura da deallocare
 *
 * @return 0, in caso di successo.
 * @return -1, in caso di errore.
 */
int conf_destroy(config_t *conf);


/**
 * @function readn
 * @brief Reimplementazione della funzione \a read di libreria.
 * La funzione non ritorna finchè non ha letto esattamente la size richiesta
 *
 * @param fd    descrittore della connessione
 * @param buf   buffer su cui inserire i dati letti
 * @param size  numero di byte da leggere
 * @param msg   messaggio da stampare in caso di errore
 *
 * @return \a size (successo) o \a 0 (connessione chiusa)
 */
static inline int readn(long fd, void *buf, size_t size, char* msg)
{
    size_t left = size;
    ssize_t r;
    char *bufptr = (char*)buf;
    while(left > 0 && buf != NULL)
    {
        if ((r = read((int)fd ,bufptr,left)) == -1)
        {
            if (errno == EINTR)
                continue;
            else if (errno == ECONNRESET) // ignoro il caso in cui la connessione sul socket è resettata
                return 1;
            else
                ERRORE(msg);
        }
        if (r == 0) return 0;   // gestione chiusura socket
        left    -= r;
        bufptr  += r;
    }
    return (int)size;
}

/**
 * @function writen
 * @brief Reimplementazione della funzione \a reawrite di libreria.
 * La funzione non ritorna finchè non ha scritto esattamente la size richiesta
 *
 * @param fd    descrittore della connessione
 * @param buf   buffer da cui leggere i dati da scrivere
 * @param size  numero di byte da scrivere
 * @param msg   messaggio da stampare in caso di errore
 *
 * @return \a size (successo) o \a 0 (connessione chiusa)
 */
static inline int writen(long fd, void *buf, size_t size, char *msg)
{
    size_t left = size;
    ssize_t r;
    char *bufptr = (char*)buf;
    while(left > 0 && buf != NULL)
    {
        if ((r = write((int)fd ,bufptr,left)) == -1)
        {
            if (errno == EINTR)
                continue;
            else if (errno == EPIPE) // ignoro il caso in cui il socket non funziona correttamente
                return 0;
            else
                ERRORE(msg)
        }
        if (r == 0) return 0;
        left    -= r;
        bufptr  += r;
    }
    return 1;
}



/**
 * @function printConfiguration
 * @brief Stampa la configurazione corrente del server
 *
 * @param stream file su cui stampare
 * @param conf  struttura di configurazione da stampare
 */
// funzione utilizzata solo in fase di debug
static inline void printConfiguration(FILE* stream, config_t *conf)
{
    fprintf(stream,"UnixPath: %s\n",conf->UnixPath);
    fprintf(stream,"MaxConnections: %d\n",conf->MaxConnections);
    fprintf(stream,"ThreadsInPool: %d\n",conf->ThreadsInPool);
    fprintf(stream,"MaxMsgSize: %d\n",conf->MaxMsgSize);
    fprintf(stream,"MaxFileSize: %d\n",conf->MaxFileSize);
    fprintf(stream,"MaxHistMsgs: %d\n",conf->MaxHistMsgs);
    fprintf(stream,"DirName: %s\n",conf->DirName);
    fprintf(stream,"StatFileName: %s\n",conf->StatFileName);
}


#endif //UTILITY_H
