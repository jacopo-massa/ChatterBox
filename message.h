/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <assert.h>
#include <string.h>
#include <config.h>
#include <ops.h>

/**
 * @file  message.h
 * @brief Contiene il formato del messaggio scambiato tra clients e server
 */


/**
 *  @struct message_hdr_t
 *  @brief header del messaggio 
 *
 *  @var op tipo di operazione richiesta al server
 *  @var sender nickname del mittente 
 */
typedef struct {
    op_t     op;   
    char sender[MAX_NAME_LENGTH+1];
} message_hdr_t;

/**
 *  @struct message_data_hdr_t
 *  @brief header della parte dati
 *
 *  @var receiver nickname del ricevente
 *  @var len lunghezza del buffer dati
 */
typedef struct {
    char receiver[MAX_NAME_LENGTH+1];
    unsigned int   len;  
} message_data_hdr_t;

/**
 *  @struct message_data_t
 *  @brief body del messaggio 
 *
 *  @var hdr header della parte dati
 *  @var buf buffer dati
 */
typedef struct {
    message_data_hdr_t  hdr;
    char               *buf;
} message_data_t;

/**
 *  @struct message_t
 *  @brief tipo del messaggio 
 *
 *  @var hdr header
 *  @var data dati
 */
typedef struct {
    message_hdr_t  hdr;
    message_data_t data;
} message_t;

/* ------ funzioni di utilità ------- */

/**
 * @function setHeader
 * @brief scrive l'header del messaggio
 *
 * @param hdr puntatore all'header
 * @param op tipo di operazione da eseguire
 * @param sender mittente del messaggio
 */
static inline void setHeader(message_hdr_t *hdr, op_t op, char *sender) {
#if defined(MAKE_VALGRIND_HAPPY)
    memset((char*)hdr, 0, sizeof(message_hdr_t));
#endif
    hdr->op  = op;
    strncpy(hdr->sender, sender, strlen(sender)+1);
}
/**
 * @function setData
 * @brief scrive la parte dati del messaggio
 *
 * @param msg puntatore al body del messaggio
 * @param rcv nickname o groupname del destinatario
 * @param buf puntatore al buffer 
 * @param len lunghezza del buffer
 */
static inline void setData(message_data_t *data, char *rcv, const char *buf, unsigned int len) {
#if defined(MAKE_VALGRIND_HAPPY)
    memset((char*)&(data->hdr), 0, sizeof(message_data_hdr_t));
#endif

    strncpy(data->hdr.receiver, rcv, strlen(rcv)+1);
    data->hdr.len  = len;
    data->buf      = (char *)buf;
}



#endif /* MESSAGE_H_ */
