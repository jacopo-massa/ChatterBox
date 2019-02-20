#!/bin/bash

if [[ $# != 2 ]]; then
    echo "usa $0 unix_path dir_name"
    exit 1
fi


# registro un po' di nickname
./client -l $1 -c pippo &
./client -l $1 -c pluto &
./client -l $1 -c minni &
./client -l $1 -c topolino &
./client -l $1 -c paperino &
./client -l $1 -c qui &
./client -l $1 -c quo &
./client -l $1 -c qua &
wait

# minni deve ricevere 8 messaggi prima di terminare
./client -l $1 -k minni -R 8 &
pid=$!

# aspetto un po' per essere sicuro che il client sia partito
sleep 1

# primo messaggio
./client -l $1 -k topolino -S "ciao da topolino":minni
if [[ $? != 0 ]]; then
    exit 1
fi
# secondo e terzo
./client -l $1 -k paperino -S "ciao da paperino":minni -S "ciao ciao!!!":minni
if [[ $? != 0 ]]; then
    exit 1
fi
# quarto
./client -l $1 -k qui -S "ciao a tutti": 
if [[ $? != 0 ]]; then
    exit 1
fi
# quinto e sesto
./client -l $1 -k quo -S "ciao a tutti":  -S "ciao da quo":minni
if [[ $? != 0 ]]; then
    exit 1
fi
# settimo ed ottavo
./client -l $1 -k qua -S "ciao a tutti":  -S "ciao da qua":minni -p
if [[ $? != 0 ]]; then
    exit 1
fi

wait $pid
if [[ $? != 0 ]]; then
    echo "ESCO8"
    exit 1
fi

# messaggio di errore che mi aspetto
OP_NICK_ALREADY=26

# provo a ri-registrare pippo
./client -l $1 -c pippo
e=$?
if [[ $((256-e)) != $OP_NICK_ALREADY ]]; then
    echo "Errore non corrispondente $e" 
    exit 1
fi

# deregistro pippo
./client -l $1 -k pippo -C pippo
if [[ $? != 0 ]]; then
    exit 1
fi
# deregistro pluto
./client -l $1 -k pluto -C pluto
if [[ $? != 0 ]]; then
    exit 1
fi

# registro pippo
./client -l $1 -c pippo
if [[ $? != 0 ]]; then
    exit 1
fi
# registro pluto
./client -l $1 -c pluto
if [[ $? != 0 ]]; then
    exit 1
fi

# pippo e pluto si scambiano files
./client -l $1 -k pippo -S "Ti mando un file":pluto -s ./client:pluto
if [[ $? != 0 ]]; then
    exit 1
fi
./client -l $1 -k pluto -S "Ti mando un file":pippo -s ./chatty:pippo -s ./libchatty.a:pippo
if [[ $? != 0 ]]; then
    exit 1
fi

# controllo che i file siano arrivati al server e che siano corretti
md51=$(md5sum ./client | cut -d " " -f 1)
md52=$(md5sum ./chatty | cut -d " " -f 1)
md53=$(md5sum ./libchatty.a | cut -d " " -f 1)
md5client=$(md5sum $2/client | cut -d " " -f 1)
md5chatty=$(md5sum $2/chatty | cut -d " " -f 1)
md5lib=$(md5sum $2/libchatty.a | cut -d " " -f 1)

if [[ $md5client != $md51 ]]; then
    echo "./client e $2/client differiscono!"
    exit 1
fi
if [[ $md5chatty != $md52 ]]; then
    echo "./chatty e $2/chatty differiscono!"
    exit 1
fi
if [[ $md5lib != $md53 ]]; then
    echo "./libchatty.a e $2/libchatty.a differiscono!"
    exit 1
fi

echo "Test OK!"
exit 0


