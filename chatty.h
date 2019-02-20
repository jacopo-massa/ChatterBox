/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.h
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief
 * Funzioni eseguite dai thread del server.
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 */


/** @mainpage Documentazione Chattebox
 * @section intro_sec Introduzione
 *
 * Chatterbox è un server concorrente che implementa una chat.
 * Tramite un socket AF_UNIX gli utenti della chat possono scambiarsi messaggi testuali e/o files
 * registrandosi al server, e collegandosi ad esso tramite opportune operazioni.
 *
 * @section op_sec Operazioni
 * Gli utenti possono effettuare diverse operazioni, che il server svolge concorrentemente utilizzando il mutlithreading.\n
 * Per conoscere le possibili operazioni consultare i file \a ops.h e \a chatty.h
 * \n \n
 * @author __**Jacopo Massa**__ \n
 * Matricola 543870 \n
 * email: <mailto:jacopomassa97@gmail.com> \n
 *
 * @copyright **Si dichiara che il contenuto dell'intero progetto è in ogni sua parte opera
       originale dell'autore**
 */

#ifndef CHATTY_H
#define CHATTY_H

#include <message.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stats.h>
#include <utility.h>
#include <icl_hash.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <threadpool.h>
#include <connections.h>

/* ----- FUNZIONI ESEGUITE DAI THREAD ------ */

/**
 * @function listener_function
 * @brief Funzione eseguita dal thread listener
 * @param[in] connfd descrittore del socket di connessione
 */
void* listener_function(void *connfd);

/**
 * @function sig_manager_function
 * @brief Funzione eseguita dal thread gestore dei segnali
 * @param[in] sigset set dei segnali da ascoltare
 */
void* sig_manager_function(void* sigset);

/**
 * @function chooseRequest
 * @brief Funzione eseguita dai thread del pool
 * @param[in] fd descrittore del client
 */
void chooseRequest(void* fd);


/* ----- OPERAZIONI EFFETTUATE DAL SERVER ----- */

/**
 * @function registerUser
 * @brief Resgistra un nuovo nickname nel server
 * @param[in] fd descrittore del client
 * @param[in] nickname id univoco del nuovo utente
 */
void registerUser(long fd, char* nickname);

/**
 * @function unregisterUser
 * @brief Deresgistra un utente dal server
 * @param[in] fd descrittore del client
 * @param[in] nickname id univoco dell'utente da deregistrare
 */
void unregisterUser(long fd, char* nickname);

/**
 * @function connectUser
 * @brief Connette un client al server
 * @param[in] fd descrittore del client
 * @param[in] nickname id univoco dell'utente da connettere
 */
void connectUser(long fd, char* nickname);

/**
 * @function getFile
 * @brief Scarica un file dal server
 * @param[in] fd descrittore del client
 * @param[in] msg messaggio ricevuto dal client
 */
void getFile(long fd, message_t msg);

/**
 * @function postFile
 * @brief Invia un file ad un nickname
 * @param[in] fd descrittore del client
 * @param[in] msg messaggio ricevuto dal client
 */
void postFile(long fd, message_t msg);

/**
 * @function postText
 * @brief Invia un messaggio testuale ad un nickname
 * @param[in] fd descrittore del client
 * @param[in] msg messaggio ricevuto dal client
 */
void postText(long fd, message_t msg);

/**
 * @function postTextAll
 * @brief Invia un messaggio testuale a tutti gli utenti
 * @param[in] fd descrittore del client
 * @param[in] msg messaggio ricevuto dal client
 */
void postTextAll(long fd, message_t msg);

/**
 * @function usrList
 * @brief Invia la lista degli utenti online
 * @param[in] fd descrittore del client
 * @param[in] sender nickname dell'utente a cui mandare la lista
 */
void usrList(long fd, char* sender);

/**
 * @function postText
 * @brief Invia la history dei messaggi di un client
 * @param[in] fd descrittore del client
 * @param[in] msg messaggio ricevuto dal client
 */
void getPrevMSGS(long fd, message_t msg);


/**
 * @function main
 * @brief Main del server
 * @param[in] argc
 * @param[in] argv
 * @return <0 se c'è stato un errore
 *          0 altrimenti
 */

#endif /* CHATTY_H */
