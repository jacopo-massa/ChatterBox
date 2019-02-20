#!/bin/bash

if [[ $# != 1 ]]; then
    echo "usa $0 unix_path"
    exit 1
fi

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
clientpid+="$! "
./client -l $1 -c "zio paperone" &
clientpid+="$! "
./client -l $1 -c "clarabella" &
clientpid+=$!

wait $clientpid

for ((i=0;i<16;++i)); do 

    # si fa lo spawn di un certo numero di processi ognuno che esegue una sequenza di operazioni con differente velocita'

    ./client -l $1 -t 200 -k topolino -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":minni -S "bbbbbbbbbbbbbbbbb":pluto -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":paperino -s client:minni -s chatty:qua -p -R 1 &
    ./client -l $1 -t 600 -k paperino -R 1  -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":minni -S "bbbbbbbbbbbbbbbbb":pluto -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":topolino -s ./libchatty.a:pluto -p &
    ./client -l $1 -t 300 -k pluto -R 1  -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":minni -S "bbbbbbbbbbbbbbbbb":pluto -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":topolino -s ./libchatty.a:minni -p &
    ./client -l $1 -t 300 -k qui -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":pluto -S "bbbbbbbbbbbbbbbbb": -S "ccccccccccccccccc": -S "ddddddddddddddddddddd": -S "eeeeeeeeeeeeeeeeeeeee": -S "fffffffffffffffff": -S "gggggggggggggggd": -S "hhhhhhhhhhhhh": -S "iiiiiiiiiiiiiiiiiiiiii": -S "llllllllllllllllll": -p &
    ./client -l $1 -t 500 -k quo -L -p -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":pluto -S "bbbbbbbbbbbbbbbbb": -S "ccccccccccccccccc": -S "ddddddddddddddddddddd": -S "eeeeeeeeeeeeeeeeeeeee": -S "fffffffffffffffff": -S "gggggggggggggggd": -S "hhhhhhhhhhhhh": -S "iiiiiiiiiiiiiiiiiiiiii": -S "llllllllllllllllll": &
    ./client -l $1 -t 500 -k pippo -L -s chatty.o:qua -s client:qua -s libchatty.a:qua -p &
    ./client -l $1 -t 200 -k qua -R 2 -s DATA/chatty.conf1:pippo -S "aaaaaaaaaaaaaaaaaaaaaaaaaa":pippo -S "bbbbbbbbbbbbbbbbbb": -S "ccccccccccccccccc": -S "ddddddddddddddddddddd": -S "eeeeeeeeeeeeeeeeeeeee": -S "fffffffffffffffff": -S "gggggggggggggggd": -S "hhhhhhhhhhhhh": -S "iiiiiiiiiiiiiiiiiiiiii": -S "llllllllllllllllll": &
    ./client -l $1 -t 100 -k minni -S "aaaaaaaaaaaaaaaaaaaaaaaaaaa":qua -S "bbbbbbbbbbbbbbbbb":pluto -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":topolino -s ./libchatty.a:pluto -p &
    ./client -l $1 -t 300 -k "zio paperone" -S "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":clarabella -S "bbbbbbbbbbbbbbbbbb": -S "ccccccccccccccccc": -S "ddddddddddddddddddddd":topolino -p &
    ./client -l $1 -t 100 -k clarabella -S "bbbbbbbbbbbbbbbbbbbbbbbbbbbbb": -R 1 -s chatty:minni -S "ccccccccccccccccc": -S "ddddddddddddddddddddd": -S "eeeeeeeeeeeeeeeeeeeee": -S "fffffffffffffffff": -S "gggggggggggggggd": -S "hhhhhhhhhhhhh": -S "iiiiiiiiiiiiiiiiiiiiii": -S "llllllllllllllllll": -p &

    for((k=0;k<5;++k)); do 
        # questi comandi falliscono tutti
	./client -l $1 -k "utente$k" -S "ciao":
	# statistiche 
	killall -USR1 chatty
    done    

    ./client -l $1 -c utente1
    ./client -l $1 -c utente2
    ./client -l $1 -k utente1 -R 5 -S connections.o:utente2 -p
    ./client -l $1 -k utente2 -R 5 -p -S chatty.o:utente1 

    ./client -l $i -C utente1
    ./client -l $i -C utente2

    wait
done

#statistiche 
killall -USR1 chatty


