#!/bin/bash

if [[ $# != 2 ]]; then
    echo "usa $0 unix_path stat_file"
    exit 1
fi

./client -l $1 -c paperino
./client -l $1 -c pippo
./client -l $1 -c pluto
./client -l $1 -c minni

./client -l $1 -k paperino -R -1 &
pid=$!

for((i=0;i<50;++i)); do 
    ./client -l $1 -k pippo -S "ciao":pluto; 
    ./client -l $1 -k pluto -S "ciao":pippo;  
    ./client -l $1 -k minni -s client:pluto -s client:pippo; 
done

# messaggio di errore che mi aspetto
OP_MSG_TOOLONG=28

./client -l $1 -k pippo -s libchatty.a:pluto
e=$?
if [[ $((256-e)) != $OP_MSG_TOOLONG ]]; then
    echo "Errore non corrispondente $e" 
    exit 1
fi

./client -l $1 -k pluto -S "1234567891011":pippo
e=$?
if [[ $((256-e)) != $OP_MSG_TOOLONG ]]; then
    echo "Errore non corrispondente $e" 
    exit 1
fi

sleep 1

./client -l $1 -k pippo -C pippo
./client -l $1 -k pluto -C pluto
./client -l $1 -k minni -C minni

killall -USR1 chatty
sleep 1

read reg online deliv notdeliv filedeliv filenotdeliv errors <<< $(tail -1 $2 | cut -d\  -f 3,4,5,6,7,8,9)

if [[ $reg != 1 || $online != 1 || $deliv != 0 || $notdeliv != 100 || $filedeliv != 0 || $filenotdeliv != 100 || $errors != 2 ]]; then 
    echo "Test FALLITO"
    exit 1
fi

echo "PAPERINO id"
echo $(pidof client)
kill -TERM $pid

echo "Test OK!"
exit 0


