/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

/**
* @file config.h
* @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
* @brief File contenente alcune define con valori massimi utilizzabili
* @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
      originale dell'autore**
*/

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH 32 /**< massima lunghezza dei nickname degli utenti. */

/* aggiungere altre define qui */

#define MAX_BUF_LENGTH  256 /**< massima lunghezza del buffer di lettura del file di configurazione. */

#define MAX_FILE_PATH 64 /**< massima lunghezza del path di un file. */

#define MAX_ICL_BUCKETS 1024 /**< numero massimo di liste di utenti nella hash table. */

#define MAX_MTX_REQ 32 /**< numero massimo di lock usate nell'invio di risposte ai client. */

#define MAX_MTX_USR 128 /**< numero massimo di lock usate per l'accesso alla hash table degli utenti. */



// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
