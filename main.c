#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <aio.h>

void execute_command(char* buf,ssize_t nread)
{
    char argz[64][nread];
    int word = 0,character = 0;

    for(int i = 0 ; i < nread;i++)
    {
        if(buf[i] == ' ' || buf[i] == '\n')
        {
            argz[word][character] = '\0';
            word++;
            character = 0;
        }
        else
        {
        argz[word][character] = buf[i];
        character++;
        }
    }

    char token[10];
    strcpy(token,argz[0]);
    printf("to slowo : %s\n",token);

    if(strcmp(token,"add")==0)
    {
        add(argz,word);
    }
    else if(strcmp(token,"end")==0)
    {
        end(argz,word);
    }
    else if(strcmp(token,"restore")==0)
    {
        restore(argz,word);
    }
    else if(strcmp(token,"list")==0)
    {
        list();
    }
    else if(strcmp(token,"exit")==0)
    {
        exit_command();
    }
    else
    {
        usage();
    }

}

int main(int argc,char **argv)
{

    char *buf = NULL;
    size_t len;

    while(1)
    {
        printf("> ");
        fflush(stdout);
        ssize_t nread =getline(&buf,&len,stdin);
        execute_command(buf,nread);
        if (nread == -1) break;
    }


}