/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file connections.c
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Implementazione delle funzioni del file connections.h
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**
 * @see connections.h
 */

#define _POSIX_C_SOURCE 200809L
#include <utility.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <string.h>
#include <message.h>
#include <connections.h>

int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
    int sfd;
    struct sockaddr_un sa;

    if(path == NULL || ntimes < 1 || secs < 1)
        return -1;

    SYSCALL( sfd = socket(AF_UNIX,SOCK_STREAM,0) , -1, "socket in openConnection");

    memset(&sa,'0', sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path)-1);

    for(int i=0; i<ntimes; i++)
    {
        if((connect(sfd,(struct sockaddr*)&sa,sizeof(sa))) == -1)
        {
            if (errno == ENOENT)
                sleep(secs); /* sock non esiste */
            else
                return -1;
        }
        else
        {
            printf("connected on %d\n",sfd);
            return sfd;
        }
    }

    return -1;
}

int readHeader(long connfd, message_hdr_t *hdr)
{
    if(connfd < 0 || hdr == NULL)
        return 0;

    ssize_t n_read;
    n_read = readn((int)connfd,&hdr->op,sizeof(op_t),"op read in readHeader");
    EQZERO(n_read);
    n_read = readn((int)connfd,hdr->sender,MAX_NAME_LENGTH+1,"sender read in readHeader");
    EQZERO(n_read);

    return 1;
}

int readData(long fd, message_data_t *data)
{
    if(fd < 0 || data == NULL)
        return 0;

    ssize_t n_read;

    n_read = readn((int)fd,data->hdr.receiver,MAX_NAME_LENGTH+1,"receiver read in readData");
    EQZERO(n_read);
    n_read = readn((int)fd,&data->hdr.len, sizeof(unsigned int),"length read in readData");
    if(data->hdr.len == 0) return 1; // il buffer potrebbe essere vuoto, ma va bene comunque
    SYSCALL(data->buf = (char*) malloc(sizeof(char)*data->hdr.len),NULL,"malloc buffer in readData");
    n_read = readn((int)fd,data->buf,data->hdr.len,"buf read in readData");
    EQZERO(n_read);

    return 1;
}

int readMsg(long fd, message_t *msg)
{
    if(fd < 0 || msg == NULL)
        return 0;

    ssize_t res_hdr, res_data;
    res_hdr = readHeader(fd,&msg->hdr);
    res_data = readData(fd,&msg->data);

    return res_hdr && res_data;
}

int sendHeader(long fd, message_hdr_t *msg)
{
    if(fd < 0 || msg == NULL)
        return 0;

    ssize_t n_write;

    n_write = writen((int)fd,&msg->op, sizeof(unsigned int),"op write in sendHeader");
    EQZERO(n_write);
    n_write = writen((int)fd,msg->sender, MAX_NAME_LENGTH+1,"sender write in sendHeader");
    EQZERO(n_write);

    return 1;
}

int sendData(long fd, message_data_t *msg)
{
    if(fd < 0 || msg == NULL)
        return 0;

    ssize_t n_write;

    n_write = writen((int)fd,msg->hdr.receiver, MAX_NAME_LENGTH+1,"receiver write in sendData");
    EQZERO(n_write);
    n_write = writen((int)fd,&msg->hdr.len, sizeof(unsigned int),"length write in sendData");
    if(msg->hdr.len == 0) return 1;
    n_write = writen(fd,msg->buf, msg->hdr.len,"buf write in sendData");
    EQZERO(n_write);

    return 1;
}

int sendRequest(long fd, message_t *msg)
{
    if(fd < 0 || msg == NULL)
        return 0;

    ssize_t res_hdr, res_data;

    res_hdr = sendHeader(fd,&msg->hdr);
    res_data = sendData(fd,&msg->data);

    return res_hdr && res_data;
}
