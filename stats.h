/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

 /**
  * @file stats.h
  * @brief Funzioni e strutture dati utili alla gestione delle statistiche del server
  */
#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>

/**
 * @struct statistics
 * @brief valori statistici delle prestazione del server
 */
struct statistics {
    unsigned long nusers;                       /**< numero di utenti registrati */
    unsigned long nonline;                      /**< numero di utenti connessi */
    unsigned long ndelivered;                   /**< numero di messaggi testuali consegnati */
    unsigned long nnotdelivered;                /**< numero di messaggi testuali non ancora consegnati */
    unsigned long nfiledelivered;               /**< numero di file consegnati */
    unsigned long nfilenotdelivered;            /**< numero di file non ancora consegnati */
    unsigned long nerrors;                      /**< numero di messaggi di errore */
};

/* aggiungere qui altre funzioni di utilita' per le statistiche */

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo.
 * @return -1 in caso di errore.
 */
static inline int printStats(FILE *fout)
{
    extern struct statistics chattyStats;

    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		chattyStats.nusers,
		chattyStats.nonline,
		chattyStats.ndelivered,
		chattyStats.nnotdelivered,
		chattyStats.nfiledelivered,
		chattyStats.nfilenotdelivered,
		chattyStats.nerrors
		) < 0) return -1;
    fflush(fout);
    return 0;
}

#endif /* MEMBOX_STATS_ */
