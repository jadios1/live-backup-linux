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

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))


/*

add /Users/janczyzewski/Documents/sopytemp/source /Users/janczyzewski/Documents/sopytemp/dest

*/

void usage()
{
    
}

void copyfiles(char* source_path,char* target_path)
{
    
    DIR *source_dir = opendir(source_path);
    DIR *target_dir = opendir(target_path);

    struct dirent *dp;
    struct stat filestat;


    if (source_dir == NULL){
        printf("Could not open current directory" );
        usage();
        return;
    }

    if (target_dir == NULL){
        printf("Could not open current directory" );
        usage();
        return;
    }


    do{
        errno = 0;
        if ((dp = readdir(source_dir)) != NULL)
        {
            if (lstat(dp->d_name, &filestat))
                ERR("lstat");
            if (S_ISDIR(filestat.st_mode))
            {
                continue;
            }
        }
    } 
    while (dp != NULL);


    if (closedir(source_dir)) ERR("closedir");
    if (closedir(target_dir)) ERR("closedir");

}   


void add(char **argz,int wordcount)
{
    char source_path[PATH_MAX];
    char target_path[PATH_MAX];
    
    strcpy(source_path,argz[0]);
    strcpy(target_path,argz[1]);
    
    char real_target_path[PATH_MAX];
    char real_source_path[PATH_MAX];
    
    if(realpath(target_path,real_target_path)==NULL)
    {
        mkdir(target_path, 0777);
        realpath(target_path,real_target_path);
    }

    if(realpath(source_path,real_source_path)==NULL) ERR("realpath2");

    if(strcmp(real_source_path,real_target_path)==0) 
    {
        usage();
        return;
    }
    //
    //DODAJ SPRAWDZANIE CZY TARGET PATH NIE JEST SUBDIRECTORY SOURCE DIRECTORY!!!!!!!!!
    //


    size_t source_length = strlen(real_source_path);
    if (strncmp(real_source_path, real_target_path, source_length) == 0) {

        if (real_target_path[source_length] == '\0' || real_target_path[source_length] == '/') {

            printf("target cannot be inside source directory.\n");
            return;
        }
    }



    copyfiles(real_source_path,real_target_path);

}

void end(char **argz,int wordcount)
{

}

void restore(char **argz,int wordcount)
{

}

void list()
{

}

void exit_func()
{
    exit(0);
}


void execute_command(char* buf,ssize_t nread)
{
    char *argz[64];
    int word = 0,character = 0,length = 0;
    char arg[1024];
    bool quote = false;
    for(int i = 0 ; i < nread;i++)
    {
        if(buf[i] == '"')
        {
            if(quote == false) quote = true;
            else quote = false;
            continue;
        }
        if((buf[i] == ' ' && quote == false) || buf[i] == '\n')
        {
            if(character > 0)
            {
            arg[character] = '\0';
            argz[word] = strdup(arg);            
            word++;
            character = 0;
            }
        }
        else
        {
            arg[character] = buf[i];
            character++;
        }
    }
    if (word == 0) return;

    if(strcmp(argz[0],"add")==0)
    {
        add(argz+1,word-1);
    }
    else if(strcmp(argz[0],"end")==0)
    {
        end(argz+1,word-1);
    }
    else if(strcmp(argz[0],"restore")==0)
    {
        restore(argz+1,word-1);
    }
    else if(strcmp(argz[0],"list")==0)
    {
        list();
    }
    else if(strcmp(argz[0],"exit")==0)
    {
        for(int i = 0; i < word;i++)
        {
            free(argz[i]);
        }
        exit_func();
    }
    else
    {
        usage();
    }
    printf("%s\n",argz[0]);

    for(int i = 0 ; i < word;i++)
    {
        free(argz[i]);
    }
}

int main(int argc,char **argv)
{

    char *buf = NULL;
    size_t len;
    printf("commands : add, end, list, restore, exit.\n");
    while(1)
    {
        printf("> ");
        fflush(stdout);
        ssize_t nread = getline(&buf,&len,stdin);
        execute_command(buf,nread);
        if (nread == -1) break; 
    }


}

