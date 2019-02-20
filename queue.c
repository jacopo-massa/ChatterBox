/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file queue.c
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Implementazione delle funzioni del file queue.h
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 * @see queue.h
 */

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <queue.h>
#include <config.h>


/**
 * @function allocNode
 * @brief Alloca memoria per un elemento della coda.
 *
 * @return puntatore alla zona di memoria allocata.
 * @return NULL, in caso di errore.
 */
static node_t *allocNode()         { return malloc(sizeof(node_t));  }

/**
 * @function allocQueue
 * @brief Alloca memoria per una coda.
 *
 * @return puntatore alla zona di memoria allocata.
 * @return NULL, in caso di errore.
 */
static queue_t *allocQueue()       { return malloc(sizeof(queue_t)); }

/**
 * @function freeNode
 * @brief Libera la memoria allocata con allocNode
 *
 * @param[in] node puntatore alla zona di memoria da liberare
 *
 */
static void freeNode(node_t *node) { free(node); }

/* ----- Interfaccia della coda ----- */

queue_t *initQueue(unsigned int qmax)
{
    queue_t *q = allocQueue();
    if (!q) return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->qlen = 0;
    q->qmax = qmax;
    q->qcurr = 0;
    return q;
}

void deleteQueue(queue_t *q)
{
    while(q->qlen > 0)
    {
        node_t *p = q->head;
        q->head = q->head->next;
        if (p->msg.data.buf)
        {
            free(p->msg.data.buf);
        }
        freeNode(p);
        q->qlen--;
    }
    free(q);
}

int push(queue_t *q, message_t msg)
{
    node_t *n = allocNode();
    setHeader(&n->msg.hdr,msg.hdr.op,msg.hdr.sender);
    setData(&n->msg.data,msg.data.hdr.receiver,msg.data.buf,msg.data.hdr.len);
    n->msg = msg;
    if(q->qcurr == q->qmax) //coda circolare, ricomincio a fare push di elementi dalla testa, sovrascrivendo i precendenti
    {
        n->next = q->head->next;
        free(q->head->msg.data.buf);
        freeNode(q->head);
        q->head = n;
        q->tail = n;
        q->qlen = q->qmax;
        q->qcurr = 1;
    }
    else
    {
        if(q->qlen == 0) //coda vuota
        {
            n->next = NULL;
            q->head = n;
            q->tail = n;
            q->qlen = q->qcurr = 1;
        }
        else
        {
            if(q->tail->next != NULL) //sovrascrivo nodo già presente
            {
                n->next = q->tail->next->next;
                free(q->tail->next->msg.data.buf);
                freeNode((void*)q->tail->next);
            }
            else //aggiungo normalmente in coda, poichè non ho ancora raggiunto la massima grandezza
            {
                n->next = NULL;
                q->qlen += 1;
            }
            q->tail->next = n;
            q->tail = n;
            q->qcurr += 1;
        }
    }
    return 0;
}


// funzione MAI utilizzata nella mia implementazione di ChatterBox
message_t pop(queue_t *q)
{
    node_t *curr = q->head;
    message_t data = curr->msg;
    //setHeader(&data.hdr, curr->msg.hdr.op, curr->msg.hdr.sender);
    //setData(&data.data, curr->msg.data.hdr.receiver, curr->msg.data.buf, curr->msg.data.hdr.len);
    q->head    = curr->next;
    q->qlen   -= 1;
    q->qcurr   = (int) q->qlen;
    assert(q->qlen>=0);
    //freeNode((void*)curr);
    return data;
}

unsigned long queueLength(queue_t *q)
{
    return q->qlen;
}

void queueStatus(FILE *stream, queue_t *q)
{
    fprintf(stream, "Elements (%ld):\n",q->qlen);
    unsigned long len = q->qlen;
    node_t *head = q->head;
    while(len > 0)
    {
        fprintf(stdout, "--> %s\n", head->msg.data.buf);
        head = head->next;
        len--;
    }
    fprintf(stdout, "\n");
}
