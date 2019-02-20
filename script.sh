#!/bin/bash

#
# membox Progetto del corso di LSO 2017/2018
#
# Dipartimento di Informatica Università di Pisa
# Docenti: Prencipe, Torquati
# 
# Studente: Jacopo Massa
# Matricola: 543870
# 
# Si dichiara che il contenuto di questo file è in ogni sua parte 
# opera originale dell'autore

# Funzione che stampa su stderr il messaggio di usage
function usage
{
	echo "Lo script va lanciato con il seguente comando:" 1>&2
	echo "$0 [-help] conffile timeval\n" 1>&2
}

# Controllo se uno dei parametri è '-help', nel caso stampo il messaggio di usage
for p; do
    if [ "$p" = "-help" ]; then
    	$(usage)
    	exit
    fi
done

# Se lo script viene lanciato con un numero di argomenti non valido, stampo il messaggio di usage
if [ $# -ne 2 ]; then
	$(usage)
	exit
fi

# Controllo che il file di configurazione esista
# Inoltre se trovo all'interno l'opzione DirName, controllo che tale opzione sia valida 
# (che indichi effettivamente una directory esistente)
if ! [ -f $1 ]; then
	printf "$1 non esiste!\n"
	exit
else
	DIRNAME=$(grep -v '^#' $1 | grep DirName | cut -f 2 -d "=")
	DIRNAME=$(echo $DIRNAME | tr -d ' ')
	if [ ! -d $DIRNAME ]; then
		printf "%s non è una directory o non esiste!\n" "$DIRNAME"
	fi
fi

#Controllo che il timeval sia effettivamente un numero, e che sia >= 0

if ! [ $2 -eq $2 2> /dev/null ] || [ $2 -lt 0 ]; then
	printf "%s non è un numero ammissibile!\n" $2
	exit
else
	TIMEVAL=$2
fi
RES=$(find $DIRNAME -mmin $((-$TIMEVAL)) ! -path $DIRNAME -exec tar -cvf chatty.tar {} + | xargs rm -vfd | wc -l)
printf "Archiviati e rimossi %s files\n" $RES

if [ $TIMEVAL -eq 0 ]; then
	for f in "$DIRNAME"/*
	do
		echo $(basename $f)
	done
fi
