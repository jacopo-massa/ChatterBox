/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file utility.c
 * @author Jacopo Massa 543870 \n( <mailto:jacopomassa97@gmail.com> )
 * @brief Implementazione delle funzioni del file utility.h
 * @copyright **Si dichiara che il contenuto di questo file è in ogni sua parte opera
       originale dell'autore**

   @see utility.h
 */

#define _POSIX_C_SOURCE 200809L
#include <utility.h>
#include <config.h>
#include <ctype.h>
#include <string.h>



void parseConfigurationFile(config_t *conf, char* filepath)
{

    conf->UnixPath      = NULL;
    conf->MaxConnections= 0;
    conf->ThreadsInPool = 0;
    conf->MaxMsgSize    = 0;
    conf->MaxFileSize   = 0;
    conf->MaxHistMsgs   = 0;
    conf->DirName       = NULL;
    conf->StatFileName  = NULL;

    FILE *fp;
    char buf[MAX_BUF_LENGTH];
    int i;
    int len = MAX_BUF_LENGTH;

    SYSCALL( fp = fopen(filepath, "r"), NULL, "fopen in parseConfigurationFile");

    while( fgets(buf,len,fp) != NULL)
    {
        i=0;
        if(buf[i] == '#' || isspace(buf[i]))
            continue;

        char* field = buf;
        char* value = buf;

        while(isalpha(buf[i]))
            i++;

        // il 'value' è tutto ciò che c'è dopo il carattere '='
        value = strchr(buf,'=');
        // ho determinato il 'field'
        buf[i] = '\0';

        // mi posiziono dopo il carattere '=' e vado avanti finché trovo spazi
        value++;
        while(isspace(*value))
            value++;

        // controllo per eliminare eventuali spazi finali nella stringa 'value'
        int vlen=0; 
        while(!isspace(value[vlen]))
            vlen++;

        //elimino il carattere '\n'
        value[vlen] = '\0'; 

        if(strcmp(field,"UnixPath") == 0)
        {
            SYSCALL(conf->UnixPath = (char*) malloc(sizeof(char)*strlen(value)+1),NULL,"malloc UnixPath in parseConfigurationFile");
            strncpy(conf->UnixPath, value, strlen(value)+1);
            continue;
        }
        if(strcmp(field,"MaxConnections") == 0)
        {
            conf->MaxConnections = atoi(value);
            continue;
        }
        if(strcmp(field,"ThreadsInPool") == 0)
        {
            conf->ThreadsInPool = atoi(value);
            continue;
        }
        if(strcmp(field,"MaxMsgSize") == 0)
        {
            conf->MaxMsgSize = atoi(value);
            continue;
        }
        if(strcmp(field,"MaxFileSize") == 0)
        {
            conf->MaxFileSize = atoi(value);
            continue;
        }
        if(strcmp(field,"MaxHistMsgs") == 0)
        {
            conf->MaxHistMsgs = atoi(value);
            continue;
        }
        if(strcmp(field,"DirName") == 0)
        {
            SYSCALL(conf->DirName = (char*) malloc(sizeof(char)*strlen(value)+1),NULL,"malloc UnixPath in parseConfigurationFile");
            strncpy(conf->DirName, value, strlen(value)+1);
            continue;
        }
        if(strcmp(field,"StatFileName") == 0)
        {
            SYSCALL(conf->StatFileName = (char*) malloc(sizeof(char)*strlen(value)+1),NULL,"malloc UnixPath in parseConfigurationFile");
            strncpy(conf->StatFileName, value, strlen(value)+1);
            continue;
        }
    }
    fclose(fp);
}

int conf_destroy(config_t *conf)
{
    if(conf == NULL)
        return -1;

    if(conf->UnixPath)      free(conf->UnixPath);
    if(conf->StatFileName)  free(conf->StatFileName);
    if(conf->DirName)       free(conf->DirName);
    free(conf);

    return 0;
}
