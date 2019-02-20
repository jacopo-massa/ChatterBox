#!/bin/bash

if [[ $# != 1 ]]; then
    echo "usa $0 unix_path"
    exit 1
fi

rm -f valgrind_out
/usr/bin/valgrind --leak-check=full ./chatty -f DATA/chatty.conf1 >& ./valgrind_out &
pid=$!

# aspetto un po' per far partire valgrind
sleep 5

# registro un po' di nickname
./client -l $1 -c pippo &
clientpid+="$! "
./client -l $1 -c pluto &
clientpid+="$! "
./client -l $1 -c minni &
clientpid+="$! "
./client -l $1 -c topolino &
clientpid+="$! "
./client -l $1 -c paperino &
clientpid+="$! "
./client -l $1 -c qui &
clientpid+="$! "
./client -l $1 -c quo &
clientpid+="$! "
./client -l $1 -c qua &
clientpid+=$!

wait $clientpid

./client -l $1 -k minni -S "Ciao a tutti, io ricevo e basta": -R -1 &
pid1=$!
./client -l $1 -k qua -S "Ciao a tutti, io ricevo e basta": -R -1 &
pid2=$!

./client -l $1 -k topolino -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":minni -S "bbbbbbbbbbbbbbbbb":pluto -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":paperino -s client:minni -s chatty:qua 

./client -l $1 -k paperino -p -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":minni -S "bbbbbbbbbbbbbbbbb":pluto -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":topolino -s ./libchatty.a:minni

./client -l $1 -k pippo -p -s listener.o:minni

# invio il segnale per generare le statistiche
kill -USR1 $pid

sleep 1

# termino i client in ascolto
kill -TERM $pid1 $pid2

sleep 1

# invio il segnale per far terminare il server
kill -TERM $pid

sleep 2

r=$(tail -10 ./valgrind_out | grep "ERROR SUMMARY" | cut -d: -f 2 | cut -d" " -f 2)

if [[ $r != 0 ]]; then
    echo "Test FALLITO"
    exit 1
fi

echo "Test OK!"
exit 0


