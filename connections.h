/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include <message.h>

/**
 * @file  connections.h
 * @brief Funzioni che implementano il protocollo tra i clients ed il server
 * @see connections.c
 */

/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server 
 *
 * @param[in] path Path del socket AF_UNIX 
 * @param[in] ntimes numero massimo di tentativi di retry
 * @param[in] secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs);

// -------- server side ----- 
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param[in] fd     descrittore della connessione
 * @param[in] hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore 
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readHeader(long connfd, message_hdr_t *hdr);
/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param[in] fd     descrittore della connessione
 * @param[in] data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readData(long fd, message_data_t *data);

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param[in] fd     descrittore della connessione
 * @param[in] data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readMsg(long fd, message_t *msg);

/* da completare da parte dello studente con altri metodi di interfaccia */


// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param[in] fd     descrittore della connessione
 * @param[in] msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg);

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param[in] fd     descrittore della connessione
 * @param[in] msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg);


/* da completare da parte dello studente con eventuali altri metodi di interfaccia */

/**
 * @function sendHeader
 * @brief Invia l'header del messaggio
 *
 * @param[in] fd     descrittore della connessione
 * @param[in] msg    puntatore all'header del messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendHeader(long fd, message_hdr_t *hdr);


#endif /* CONNECTIONS_H_ */
