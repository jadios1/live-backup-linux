#include "utils.h"
#include "file_ops.h"
#include "common.h"
#include "inotify.h"

// #include <linux/limits.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))


/*

add /Users/janczyzewski/Documents/sopytemp/source /Users/janczyzewski/Documents/sopytemp/dest /Users/janczyzewski/Documents/sopytemp/dest2

add /home/jadios/Documents/sop_temp/source /home/jadios/Documents/sop_temp/dest  

*/



void add(char **argz,int wordcount)
{
    char source_path[PATH_MAX];
    char target_path[PATH_MAX];
    
    pid_t pid;

    char real_source_path[PATH_MAX];
    char real_target_path[PATH_MAX];


    strcpy(source_path,argz[0]);


    if (realpath(source_path, real_source_path) == NULL) {
        ERR("cannot resolve source path");
    }
    
    for(int i = 1 ; i < wordcount;i++)
    {
        strcpy(target_path,argz[i]);
        if((directory_checker(target_path,real_source_path))==true)
        {
            if((pid = fork())<0)ERR("fork");

            if(pid==0)
            {
                realpath(target_path,real_target_path);
                copyfiles(real_source_path,target_path,real_source_path,real_target_path);

                AllWatches allwatches;
                allwatches.fd = inotify_init();
                allwatches.capacity = INITIAL_WATCH_CAPACITY;
                allwatches.count = 0;
                allwatches.watches = malloc(sizeof(Watch) * allwatches.capacity);
                strcpy(allwatches.root_src,real_source_path);
                strcpy(allwatches.root_tgt,real_target_path);
                add_watches(&allwatches, real_source_path);
                exit(EXIT_SUCCESS);
            }


        }
    }
}


void usage_func()
{
    printf("usagebruh\n");
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
    int word = 0,character = 0;
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
        usage_func();
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
    
    return 0;


}
