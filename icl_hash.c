/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file icl_hash.c
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Implementazione delle funzioni del file icl_hash.h
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 * @see icl_hash.h
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <icl_hash.h>
#include <utility.h>
#include <config.h>

#include <limits.h>


#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

/**
 * @function hash_pjw
 * @brief Una semplice funzione di hash su stringhe
 *
 * @param[in] key chiave di cui si vuole ottenere l'hash
 *
 * @return indice di hash
 * @return 0, in caso di errore
 */
static unsigned int hash_pjw(void* key)
{
    char *datum = (char *)key;
    unsigned int hash_value, i;

    if(!datum) return 0;

    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value);
}

/**
 * @function string_compare
 * @brief Ridefinizione della funzione strcmp di libreria
 *
 * @param[in] a prima stringa da confrontare
 * @param[in] b seconda stringa da confrontare
 *
 * @return valore del confronto tra i due parametri
 */
static int string_compare(void* a, void* b)
{
    return (strcmp( (char*)a, (char*)b ) == 0);
}

icl_hash_t *
icl_hash_create( int nbuckets, int npartitions, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) )
{
    icl_hash_t *ht;
    int i;

    ht = (icl_hash_t*) malloc(sizeof(icl_hash_t));
    if(!ht) return NULL;

    if(npartitions < 1 || npartitions > nbuckets) return NULL;
    ht->npartitions = npartitions;

    ht->nentries = 0;
    ht->buckets = (icl_entry_t**)malloc(nbuckets * sizeof(icl_entry_t*));
    if(!ht->buckets) return NULL;

    ht->nbuckets = nbuckets;
    for(i=0;i<ht->nbuckets;i++)
        ht->buckets[i] = NULL;

    ht->hash_function = hash_function ? hash_function : hash_pjw;
    ht->hash_key_compare = hash_key_compare ? hash_key_compare : string_compare;

    return ht;
}

icl_entry_t *icl_hash_find(icl_hash_t *ht, void* key)
{
    icl_entry_t* curr;
    unsigned int hash_val;

    if(!ht || !key) return NULL;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key))
            return curr;

    return NULL;
}

icl_entry_t *icl_hash_insert(icl_hash_t *ht, void* key, int fd)
{
    extern config_t* conf;
    icl_entry_t *curr;
    unsigned int hash_val;

    if(!ht || !key) return NULL;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key))
            return(NULL); /* key already exists */

    /* if key was not found */
    curr = (icl_entry_t*)malloc(sizeof(icl_entry_t));
    if(!curr) return NULL;

    curr->key = strndup(key,strlen(key)+1);
    curr->fd = fd;
    curr->online = 1;
    curr->queue = initQueue((unsigned int)conf->MaxHistMsgs);
    curr->next = ht->buckets[hash_val]; /* add at start */

    ht->buckets[hash_val] = curr;
    ht->nentries++;

    return curr;
}

int icl_hash_delete(icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_queue)(queue_t*))
{
    icl_entry_t *curr, *prev;
    unsigned int hash_val;

    if(!ht || !key) return -1;
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    prev = NULL;
    for (curr=ht->buckets[hash_val]; curr != NULL; )  {
        if ( ht->hash_key_compare(curr->key, key)) {
            if (prev == NULL) {
                ht->buckets[hash_val] = curr->next;
            } else {
                prev->next = curr->next;
            }
            if (*free_key && curr->key) (*free_key)(curr->key);
            if (*free_queue && curr->queue) (*free_queue)(curr->queue);
            ht->nentries--;
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

int icl_hash_destroy(icl_hash_t *ht, void (*free_key)(void*), void (*free_queue)(queue_t*))
{
    icl_entry_t *bucket, *curr, *next;
    int i;

    if(!ht) return -1;

    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            next=curr->next;
            if (*free_key && curr->key) (*free_key)(curr->key);
            if (*free_queue && curr->queue && queueLength(curr->queue)!=0) (*free_queue)(curr->queue);
            free(curr);
            curr=next;
        }
    }

    if(ht->buckets) free(ht->buckets);
    if(ht) free(ht);

    return 0;
}

int icl_hash_dump(FILE* stream, icl_hash_t* ht)
{
    icl_entry_t *bucket, *curr;
    int i;

    if(!ht) return -1;

    for(i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr=bucket; curr!=NULL; ) {
            if(curr->key)
            {
                fprintf(stream, "%s (%d)\n", (char *) curr->key, curr->online);
                queueStatus(stream,curr->queue);
            }
            curr=curr->next;
        }
    }

    return 0;
}

int icl_hash_get_partition(icl_hash_t *ht, void* key)
{
    if(!ht || !key) return -1;
    return ((*ht->hash_function)(key) % ht->nbuckets) % ht->npartitions;
}

char* icl_hash_get_onlineusers(icl_hash_t *ht, int size)
{
    if(!ht || size < 0) return NULL;

    int i, onlinecount=0;
    icl_entry_t *bkt, *curr;
    char *res;
    int length = MAX_NAME_LENGTH+1;

    SYSCALL(res = (char*) calloc((size_t) size*length,sizeof(char)),NULL,"calloc res in icl_hash_get_onlineusers")

    char *app = res;
    for(i=0; i < ht->nbuckets; i++)
    {
        bkt = ht->buckets[i];
        for(curr=bkt; curr!=NULL; curr=curr->next)
        {
            if(curr->online == 1 && curr->key!=NULL && strlen(curr->key)!=0)
            {
                if(onlinecount > 0) res += length;

                onlinecount++;
                strncat(res,curr->key,length);
            }
        }
    }
    app[onlinecount*length]='\0';
    res=app;
    return res;
}

int icl_hash_isOnline(icl_hash_t *ht, void *key)
{
    if(!ht || !key) return -1;
    icl_entry_t *usr;
    usr = icl_hash_find(ht,key);

    if(usr == NULL) return -1;

    return usr->online;
}

int icl_hash_isRegistered(icl_hash_t *ht, void *key)
{
    if(!ht || !key) return -1;
    icl_entry_t *usr;
    usr = icl_hash_find(ht,key);

    if(usr == NULL) return 0;

    return 1;
}

int icl_hash_set_online(icl_hash_t *ht, void *key, int fd)
{
    if(!ht || !key || fd < 0) return -1;

    icl_entry_t* curr;
    unsigned int hash_val;
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key))
        {
            if(curr->online == 0) //se non è online, aggiorno il suo stato
                curr->online=1;

            curr->fd = fd; //in ogni caso, aggiorno il suo fd
            return 0;
        }

    return -1;
}

int icl_hash_set_offline(icl_hash_t *ht, int fd)
{
    extern pthread_mutex_t mtx_users[MAX_MTX_USR];
    if(!ht || fd < 0) return -1;
    int i,partition;
    icl_entry_t *bkt, *curr;

    for(i=0; i < ht->nbuckets; i++)
    {
        bkt = ht->buckets[i];
        partition = i % ht->npartitions;
        LOCK(mtx_users[partition],"mtx_users in icl_hash_set_offline");
        for (curr = bkt; curr != NULL; curr = curr->next)
        {
            if (fd == curr->fd)
            {
                curr->online = 0;
                curr->fd = -1;
                UNLOCK(mtx_users[partition],"mtx_users in icl_hash_set_offline");
                return 0;
            }
        }
        UNLOCK(mtx_users[partition],"mtx_users in icl_hash_set_offline");
    }
    return -1;
}

int icl_hash_addMessage(icl_hash_t *ht, void *key, message_t msg)
{
    if(!ht || !key)
        return -1;

    icl_entry_t *usr;
    usr = icl_hash_find(ht,key);

    if(usr == NULL) return -1;
    return push(usr->queue,msg);
}


