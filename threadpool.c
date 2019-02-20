/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file threadpool.c
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Implementazione delle funzioni del file threadpool.h
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 * @see threadpool.h
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <threadpool.h>
#include <utility.h>
#include <string.h>

/**
 * @function threadpool_thread
 * @brief funzione eseguita da tutti i thread del pool
 *
 * @param[in] threadpool threadpool di cui fa parte il thread
 *
 * @return (void*)NULL.
 */
static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    for(;;)
    {
        // uso la lock per poter attendere sulla variabile di condizione
        LOCK(pool->lock,"pool->lock in threadpool_thread");

        /* Mi metto in attesa sulla variabile di condizione,
         * controllando che nel frattempo il pool non stia terminando
         */
        while((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        // ricontrollo un'eventuale terminazione dopo essere tornato dalla wait
        if((pool->shutdown == immediate_shutdown) ||
           ((pool->shutdown == graceful_shutdown) &&
            (pool->count == 0))) {
            break;
        }

        // estraggo un lavoro dalla coda condivisa
        task.function = pool->queue[pool->head].function;
        task.arg = pool->queue[pool->head].arg;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        UNLOCK(pool->lock,"pool->lock in threadpool_thread");

        // eseguo il lavoro estratto dalla coda
        (*(task.function))(task.arg);
    }

    pool->started--;

    UNLOCK(pool->lock,"pool->lock in threadpool_thread");
    pthread_exit(NULL);
}

threadpool_t *threadpool_create(int thread_count, int queue_size)
{
    threadpool_t *pool;
    int i;

    if(thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)
        return NULL;

    if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL)
        goto err;

    // inizializzo del pool
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;

    // alloco memoria per i thread e per la coda condivisa
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (threadpool_task_t *)malloc
        (sizeof(threadpool_task_t) * queue_size);

    /* Inizializzo la mutex e la variabile di condizione */
    if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
       (pthread_cond_init(&(pool->notify), NULL) != 0) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL)) {
        goto err;
    }

    // faccio partire l'esecuzione di ogni thread con la funzione 'threadpool_thread'
    for(i = 0; i < thread_count; i++) {
        if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0)
        {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;

 err:
    if(pool) //in caso di errore libero la memoria allocata
    {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *arg)
{
    int err = 0;
    int next;

    if(pool == NULL || function == NULL)
        return threadpool_invalid;

    if(pthread_mutex_lock(&(pool->lock)) != 0)
        return threadpool_lock_failure;

    next = (pool->tail + 1) % pool->queue_size;

    do
    {
        // controllo che la coda di lavori non sia piena
        if(pool->count == pool->queue_size)
        {
            err = threadpool_queue_full;
            break;
        }

        // controllo che il pool non stia terminando
        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        // aggiungo il lavoro alla coda
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].arg = arg;
        pool->tail = next;
        pool->count += 1;

        // segnalo ai thread del pool che c'è qualcosa nella coda
        if(pthread_cond_signal(&(pool->notify)) != 0)
        {
            err = threadpool_lock_failure;
            break;
        }
    } while(0);

    if(pthread_mutex_unlock(&pool->lock) != 0)
        err = threadpool_lock_failure;

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if(pool == NULL)
        return threadpool_invalid;

    if(pthread_mutex_lock(&(pool->lock)) != 0)
        return threadpool_lock_failure;

    do
    {
        // controllo che il pool non stia terminando
        if(pool->shutdown)
        {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ?
            graceful_shutdown : immediate_shutdown;

        // risveglio tutti i thread del pool
        if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = threadpool_lock_failure;
            break;
        }

        // attendo la terminazione di tutti i thread
        for(i = 0; i < pool->thread_count; i++) {
            if(pthread_join(pool->threads[i], NULL) != 0) {
                err = threadpool_thread_failure;
            }
        }
    } while(0);

    /* Se tutto è andato bene dealloco il threadpool */
    if(!err) {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    // controllo che effettivamente sono riuscito ad allocare il pool
    if(pool->threads) {
        free(pool->threads);
        free(pool->queue);

        /* Avendo allocato i thread dopo aver inizializzato
         * la mutex e la variabile di condizione, sono sicuro che esse sono
         * inizializzate.
         *
         * Effettuo una LOCK sulla mutex solo per sicurezza */
        LOCK(pool->lock,"pool->lock in threadpool free");
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
    return 0;
}
