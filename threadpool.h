/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file threadpool.h
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief **Funzioni e strutture dati per la gestione di un pool di thread
 * @copyright Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 */

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#define MAX_THREADS 64 /**< massimo numero di thread che il pool può contenere. */
#define MAX_QUEUE 65536 /**< lunghezza massima della coda condivisa fra i thread del pool. */

#include <pthread.h>

/**
 * @typedef threadpool_task_t
 * @brief Ridefinizione della struttura threadpool_task_s
 *
 *  @struct threadpool_task_s
 *  @brief Elemento della coda dei lavori
 *
 *  @param[in] function puntatore alla funzione che dovrà essere eseguita
 *  @param[in] argument argomento da passare a function
 */
typedef struct threadpool_task_s
{
    void (*function)(void *);
    void *arg;
} threadpool_task_t;

/**
 * @typedef threadpool_t
 * @brief Ridefinizione della struttura threadpool_s
 *
 *  @struct threadpool_s
 *  @brief Struttura dati threadpool
 *
 *  @param[in] lock         variabile mutex per l'accesso in mutua esclusione alla coda dei lavori
 *  @param[in] notify       variabile di condizione per notificare i thread
 *  @param[in] threads      Array contenente gli ID dei thread del pool
 *  @param[in] queue        coda contenente i lavori da eseguire
 *  @param[in] thread_count numero di thread
 *  @param[in] queue_size   grandezza della coda dei lavori
 *  @param[in] head         indice del primo elemento
 *  @param[in] tail         indice dell'ultimo elemento
 *  @param[in] count        numero di lavori da eseguire
 *  @param[in] shutdown     flag che indica se il pool sta terminando
 *  @param[in] started      numero di thread avviati
 */
typedef struct threadpool_s
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
}threadpool_t;

/**
 * @typedef threadpool_shutdown_t
 * @brief Ridefinizione della enum threadpool_shutdown_s
 */
/**
 * @enum threadpool_shutdown_s
 * @brief Tipologie di terminazione del threadpool
 */
typedef enum threadpool_shutdown_s
{
    immediate_shutdown = 1, /**< termina immediatamente a causa di un errore */
    graceful_shutdown  = 2 /**< termina dopo aver processato i lavori in sospeso */
} threadpool_shutdown_t;


/**
 * @typedef threadpool_error_s
 * @brief Ridefinizione della enum threadpool_error_t
 */
/**
 * @enum threadpool_error_s
 * @brief Tipologie di errori con cui il threadpool può terminare
 */
typedef enum threadpool_error_s
{
    threadpool_invalid        = -1, /**< argomenti delle funzioni non validi */
    threadpool_lock_failure   = -2, /**< errore sulla lock o sulla variabile di condizione */
    threadpool_queue_full     = -3, /**< coda di lavori piena */
    threadpool_shutdown       = -4, /**< il pool sta terminando */
    threadpool_thread_failure = -5  /**< problema legato ad un thread del pool */
} threadpool_error_t;

/**
 * @typedef threadpool_destroy_flags_s
 * @brief Ridefinizione della enum threadpool_destroy_flags_t
 */
/**
 * @enum threadpool_destroy_flags_s
 * @brief Flag per la terminazione del threadpool
 */
typedef enum threadpool_destroy_flags_s{
    threadpool_graceful       = 1 /**< ridenominazione del valore 1 */
} threadpool_destroy_flags_t;

/**
 * @function threadpool_create
 * @brief Crea un pool di thread
 *
 * @param[in] thread_count numero di thread da inserire nel pool.
 * @param[in] queue_size   grandezza della coda dei lavori.
 *
 * @return puntatore al threadpool creato.
 * @return NULL, in caso di errore.
 */
threadpool_t *threadpool_create(int thread_count, int queue_size);

/**
 * @function threadpool_add
 * @brief Aggiunge un nuovo lavoro alla coda dei lavori del threadpool
 *
 * @param[in] pool     threadpool in cui aggiungere il lavoro
 * @param[in] function puntatore alla funzione che dovrà essere eseguita
 * @param[in] arg argomento da passare alla funzione
 *
 * @return 0 in caso di successo.
 * @return <0 in caso di errore.
 *
 * @see threadpool_error_t per i codici di errore.
 */
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *arg);

/**
 * @function threadpool_destroy
 * @brief Ferma i thread e lascia la memoria occupata dal pool in uno stato consistente
 * @param[in] pool  threadpool da distruggere
 * @param[in] flags Flag per la terminazione
 *
 * I valori ammissibili per flags sono 0 (default) e threadpool_graceful.\n
 * Nel secondo caso il threadpool non accetta più lavori ma processa tutto quelli
 * in sospeso prima di terminare.
 */
int threadpool_destroy(threadpool_t *pool, int flags);


/**
 * @function threadpool_free
 * @brief Dealloca la memoria allocata con threadpool_create
 * @param[in] pool  threadpool da distruggere
 *
 * @return 0, in caso di successo.
 * @return -1, in caso di errore.
 */
int threadpool_free(threadpool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* _THREADPOOL_H_ */
