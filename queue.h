/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file queue.h
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Funzioni e strutture per la gestione di una coda circolare
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <message.h>
#include <stdio.h>

/**
 * @struct node_s
 * @brief Elemento della coda.
 *
 * @param[in] msg messaggio della history
 * @param[in] next puntatore al prossimo messaggio della history
 */
struct node_s
{
    message_t   msg;
    struct node_s * next;
};


/**
 * @typedef node_t
 * @brief Ridefinizione della struttura node_s */
typedef struct node_s node_t;

/**
 * @struct queue_s
 * @brief Struttura dati coda.
 *
 * @param[in] head elemento iniziale
 * @param[in] tail ultimo elemento inserito
 * @param[in] qlen lunghezza corrente della coda
 * @param[in] qcurr posizione dell'ultimo elemento inserito
 * @param[in] qmax numero massimo di elementi che la coda può contenere
 */
struct queue_s
{
    node_t        *head;
    node_t        *tail;
    unsigned long  qlen;
    unsigned  int qcurr;
    unsigned int   qmax;
};


/**
 * @typedef queue_t
 * @brief Ridefinizione della struttura queue_s */
typedef struct queue_s queue_t;


/** 
 * @function initQueue
 * @brief Alloca ed inizializza una coda.
 *
 * @param[in] qmax numero massimo di elementi che la coda può contenere
 *
 * @return puntatore alla coda allocata
 * @return NULL, in caso di errore.
 */
queue_t *initQueue(unsigned int qmax);

/**
 * @function deleteQueue
 * @brief Cancella una coda allocata con initQueue.
 *  
 * @param[in] puntatore alla coda da cancellare
 */
void deleteQueue(queue_t *q);

/**
 * @function push
 * @brief Inserisce un messaggio nella coda.
 *
 * @param[in] puntatore alla coda in cui inserire
 * @param[in] msg messaggio da inserire nella coda
 *  
 * @return 0
 */
int push(queue_t *q, message_t msg);

/**
 * @function pop
 * @brief Legge un messaggio dalla coda.
 *
 * @param[in] puntatore alla coda da cui leggere
 *
 * @return elemento in testa alla coda
 */
message_t pop(queue_t *q);

/**
 * @function queueLength
 * @brief Ritorna la lunghezza della coda.
 *
 * @param[in] puntatore alla coda
 *
 * @return lunghezza della coda.
 */
unsigned long queueLength(queue_t *q);


/**
 * @function queueStatus
 * @brief Stampa su il contenuto della coda
 *
 * @param[in] stream puntatore al file su cui stampare
 * @param[in] q puntatore alla coda da stampare
 */
void queueStatus(FILE* stream, queue_t *q);


#endif /* QUEUE_H_ */
