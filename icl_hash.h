/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file icl_hash.h
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Funzioni e strutture dati per la gestione di una hash table contenente utenti di un server
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 */

#ifndef ICL_HASH_H
#define ICL_HASH_H

#include <stdio.h>
#include <queue.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/**
 * @typedef icl_entry_t
 * @brief Ridefinizione della struttura icl_entry_s
 *
 * @struct icl_entry_s
 * @brief Elemento della tabella hash
 *
 * @param[in] key nickname dell'utente
 * @param[in] fd descrittore della connessione
 * @param[in] online flag di stato dell'utente
 * @param[in] queue history dei messaggi
 * @param[in] next puntatore all'utente successivo
 */
typedef struct icl_entry_s
{
    void* key;
    int fd;
    int online;
    queue_t *queue;
    struct icl_entry_s* next;
}icl_entry_t;


/**
 * @typedef icl_hash_t
 * @brief Ridefinizione della struttura icl_hash_s
 *
 * @struct icl_hash_s
 * @brief Struttura dati hash table
 *
 * @param[in] npartitions numero di partizioni
 * @param[in] nbuckets lunghezza della tabella
 * @param[in] nentries
 * @param[in] buckets liste di utenti
 * @param[in] hash_function puntatore alla funzione di hash della tabella
 * @param[in] hash_key_compare puntatore alla funzione di confronto tra chiavi
 */
typedef struct icl_hash_s
{
    int npartitions;
    int nbuckets;
    int nentries;
    icl_entry_t **buckets;
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
}icl_hash_t;

/**
 * @function icl_hash_create
 * @brief Crea una nuova hash table.
 *
 * @param[in] nbuckets numero di liste di utenti da creare
 * @param[in] npartitions numero di partizione della hash table
 * @param[in] hash_function puntatore alla funzione hash da usare
 * @param[in] hash_key_compare puntatore alla funzione di confronto tra chiavi
 *
 * @return puntatore alla nuova hash table.
 * @return NULL in caso di errore.
 */
icl_hash_t *
icl_hash_create( int nbuckets, int npartitions, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) );

/**
 * @function icl_hash_find
 * @brief Cerca un elemento nella hash table
 *
 * @param[in] ht hash table in cui cercare
 * @param[in] key chiave dell'elemento da trovare
 *
 * @return puntatore all'elemento corrispondente alla chiave.
 * @return NULL in caso di errore.
 */
icl_entry_t *icl_hash_find(icl_hash_t *ht, void* key);

/**
 * @function icl_hash_insert
 * @brief Inserisce un elemento nella hash table
 *
 * @param[in] ht hash table in cui inserire l'elemento
 * @param[in] key chiave del nuovo elemento
 *
 * @return puntatore all'elemento inserito
 * @return NULL in caso di errore.
 */
icl_entry_t *icl_hash_insert(icl_hash_t *ht, void* key, int fd);

/**
 * @function icl_hash_delete
 * @brief Libera la memoria di un elemento della hash table
 *
 * @param[in] ht hash table in cui eliminare un elemento
 * @param[in] key chiave dell'elemento da eliminare
 * @param[in] free_key puntatore alla funzione che libera la memoria della key
 * @param[in] free_queue puntatore alla funzione che libera la memoria della history
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */
int icl_hash_delete(icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_queue)(queue_t*));

/**
 * @function icl_hash_destroy
 * @brief Libera la memoria occupata dalla hash table
 *
 * @param[in] ht hash table da eliminare
 * @param[in] free_key puntatore alla funzione che libera la memoria della key
 * @param[in] free_queue puntatore alla funzione che libera la memoria della history
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */

int icl_hash_destroy(icl_hash_t *ht, void (*free_key)(void*), void (*free_queue)(queue_t*));

/**
 * @function icl_hash_dump
 * @brief Stampa il contenuto della hash table sul file passato come argomento
 *
 * @param[in] stream puntatore al file su cui stampare la hash table
 * @param[in] ht hash table da stampare
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */
int icl_hash_dump(FILE* stream, icl_hash_t* ht);

/**
 * @function icl_hash_get_partition
 * @brief Individua la partizione di un elemento
 *
 * @param[in] ht hash table da analizzare
 * @param[in] key chiave dell'elemento
 *
 * @return il numero della partizione
 * @return -1 in caso di errore.
 */
int icl_hash_get_partition(icl_hash_t *ht, void* key);

/**
 * @function icl_hash_get_onlineusers
 * @brief Costruisce la lista di utenti online
 *
 * @param[in] ht hash table da analizzare
 * @param[in] nowonline numero di utenti attualmente online
 *
 * @return lista degli utenti online
 * @return NULL in caso di errore.
 */
char* icl_hash_get_onlineusers(icl_hash_t *ht, int nowonline);

/**
 * @function icl_hash_isOnline
 * @brief Indica lo stato di un utente (online/offline)
 *
 * @param[in] ht hash table da analizzare
 * @param[in] key nickname dell'utente
 *
 * @return lo stato dell'utente
 * @return NULL in caso di errore.
 */
int icl_hash_isOnline(icl_hash_t *ht, void *key);

/**
 * @function icl_hash_isRegistered
 * @brief Indica lo stato di un utente (registrato / non registrato)
 *
 * @param[in] ht hash table in cui si trova l'elemento
 * @param[in] key nickname dell'utente
 *
 * @return lo stato dell'utente
 * @return NULL in caso di errore.
 */
int icl_hash_isRegistered(icl_hash_t *ht, void *key);

/**
 * @function icl_hash_set_online
 * @brief Cambia lo stato di un utente in ONLINE
 *
 * @param[in] ht hash table in cui si trova l'elemento
 * @param[in] key nickname dell'utente
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */

int icl_hash_set_online(icl_hash_t *ht, void *key, int fd);

/**
 * @function icl_hash_set_offline
 * @brief Cambia lo stato di un utente in OFFLINE
 *
 * @param[in] ht hash table in cui si trova l'elemento
 * @param[in] key nickname dell'utente
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */
int icl_hash_set_offline(icl_hash_t *ht, int fd);

/**
 * @function icl_hash_addMessage
 * @brief Aggiunge un messaggio alla history di un utente
 *
 * @param[in] ht hash table in cui si trova l'elemento
 * @param[in] key nickname dell'utente
 * @param[in] msg messaggio da aggiungere alla history
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */
int icl_hash_addMessage(icl_hash_t *ht, void *key, message_t msg);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* ICL_HASH_H */
