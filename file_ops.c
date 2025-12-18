#include "file_ops.h"


void copysymlink(char* source_file_path, char* target_file_path, char* root_source_path, char* root_target_path) {
    
    
    char link_content[PATH_MAX];
    ssize_t len = readlink(source_file_path, link_content, sizeof(link_content) - 1);
    
    if (len == -1) {
        perror("readlink");
        return;
    }
    
    
    link_content[len] = '\0';

    size_t root_src_len = strlen(root_source_path);
    if (link_content[0] == '/' && strncmp(link_content, root_source_path, root_src_len) == 0) {
        
        char new_target_link[PATH_MAX];
        snprintf(new_target_link, PATH_MAX, "%s%s", root_target_path, link_content + root_src_len);
        
        if (symlink(new_target_link, target_file_path) == -1) {
            perror("symlink");
        }
    } else {
        if (symlink(link_content, target_file_path) == -1) {
            perror("symlink");
        }
    }
}



void copyfile(char* source_file_path, char* target_file_path,struct stat stat)
{
    int fd_to;
    int fd_from;
    ssize_t nread;
    char buf[4096];
    
    
    if((fd_from = open(source_file_path, O_RDONLY))<0) ERR("open");
    if((fd_to = open(target_file_path, O_WRONLY | O_CREAT | O_TRUNC, stat.st_mode))<0) ERR("open");    


    while (nread = read(fd_from, buf, sizeof(buf)), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
        } while (nread > 0);
    }
    
    if (nread < 0) ERR("read");

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            ERR("file close");
            return;
        }
        close(fd_from);
    }
}



void copyfiles(char* source_path,char* target_path,char* root_source_path, char* root_target_path)
{
    
    DIR *source_dir = opendir(source_path);

    if (source_dir == NULL){
        printf("Could not open current directory" );
        return;
    }

    struct dirent *dp;
    struct stat filestat;

    char source_file_path[PATH_MAX];
    char target_file_path[PATH_MAX];


    //dodaj obsluge czy folder istnieje



    while ((dp = readdir(source_dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
        continue;
    }
        if(snprintf(source_file_path,PATH_MAX,"%s/%s",source_path,dp->d_name)<0)ERR("snprinf soruce_full_file_path");
        if(snprintf(target_file_path,PATH_MAX,"%s/%s",target_path,dp->d_name)<0)ERR("snprinf target_full_file_path");


        if (lstat(source_file_path, &filestat))
            ERR("lstat");

        
        if(S_ISREG(filestat.st_mode))
        {
            copyfile(source_file_path,target_file_path,filestat);
            
        }
        else if (S_ISDIR(filestat.st_mode))
        {
            if(mkdir(target_file_path, 0777)==-1)ERR("mkdir");
            copyfiles(source_file_path,target_file_path,root_source_path,root_target_path);
        }
        else if(S_ISLNK(filestat.st_mode))
        {
            copysymlink(source_file_path,target_file_path,root_source_path,root_target_path);
        }
    }


    if (closedir(source_dir)) ERR("closedir");

}   

