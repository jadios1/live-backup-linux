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
        
        unlink(target_file_path);
        
        if (symlink(new_target_link, target_file_path) == -1) 
        {
            perror("symlink");
        }
    } 
    else 
    {
        unlink(target_file_path);
        if (symlink(link_content, target_file_path) == -1) 
        {
            perror("symlink");
        }
    }
}



void copyfile(char* source_file_path, char* target_file_path, struct stat st)
{
    int fd_to, fd_from;
    ssize_t nread;
    char buf[4096];
    
    if((fd_from = open(source_file_path, O_RDONLY)) < 0) {
        perror("open source");
        return; 
    }

    if((fd_to = open(target_file_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777)) < 0) {
        perror("open target");
        close(fd_from);
        return;
    }

    while ((nread = read(fd_from, buf, sizeof(buf))) > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;
        
        while (nread > 0) {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) 
            {
                nread -= nwritten;
                out_ptr += nwritten;
            } 
            else 
            {
                if (errno == EINTR) continue;
                perror("write error");
                close(fd_from);
                close(fd_to);
                return; 
            }
        }
    }
    
    if (nread < 0)
    {
        perror("read error");
    }

    close(fd_from);

    if (close(fd_to) < 0)
    {
        perror("close target");
    }
}



void copyfiles(char* source_path,char* target_path,char* root_source_path, char* root_target_path)
{
    
    DIR *source_dir = opendir(source_path);

    if (source_dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent *dp;
    struct stat filestat;

    char source_file_path[PATH_MAX];
    char target_file_path[PATH_MAX];


    while ((dp = readdir(source_dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        if(snprintf(source_file_path,PATH_MAX,"%s/%s",source_path,dp->d_name)<0)ERR("snprinf soruce_full_file_path");
        if(snprintf(target_file_path,PATH_MAX,"%s/%s",target_path,dp->d_name)<0)ERR("snprinf target_full_file_path");


        if (lstat(source_file_path, &filestat))
        {
            perror("lstat");
            continue;
        }

        
        if(S_ISREG(filestat.st_mode))
        {
            copyfile(source_file_path,target_file_path,filestat);
            
        }
        else if (S_ISDIR(filestat.st_mode))
        {
            if(mkdir(target_file_path, 0777)==-1)
            {
                if (errno != EEXIST) 
                {
                    perror("mkdir failed");
                    continue;
                }
            }
            copyfiles(source_file_path,target_file_path,root_source_path,root_target_path);
        }
        else if(S_ISLNK(filestat.st_mode))
        {
            copysymlink(source_file_path,target_file_path,root_source_path,root_target_path);
        }
    }


    if (closedir(source_dir)) ERR("closedir");

}   

void remove_recursive(const char *path) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {

        if (errno != ENOENT) 
        {
            perror("lstat");
        }
        return;
    }

    if (S_ISDIR(st.st_mode)) 
    {
        DIR *d = opendir(path);
        if (!d) {
            
            if (errno != ENOENT) perror("opendir");
            return;
        }

        struct dirent *p;
        while ((p = readdir(d))) {
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            char buf[PATH_MAX];
            snprintf(buf, sizeof(buf), "%s/%s", path, p->d_name);
            remove_recursive(buf);
        }
        closedir(d);
        
        if (rmdir(path) == -1) {
            
            if (errno != ENOENT) perror("rmdir");
        
        }
    } 
    else 
    {
        if (unlink(path) == -1) 
        {
            if(errno != ENOENT) perror("unlink");        
        }
    }
}

