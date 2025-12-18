#include "inotify.h"



void add_watch(AllWatches *allwatches,char *path)
{
    int wd;
    if((wd= inotify_add_watch(allwatches->fd,path,IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM))==-1)
    {
        fprintf(stderr,"error with inotify adding single watch");
        return;
    }
    if (allwatches->count >= allwatches->capacity) {
        allwatches->capacity *= 2;
        allwatches->watches = realloc(allwatches->watches, sizeof(Watch) * allwatches->capacity);
    }

    allwatches->watches[allwatches->count].wd = wd;
    strcpy(allwatches->watches[allwatches->count].path, path);
    allwatches->count++;

}

void path_changer(char *src_path, AllWatches *watches, char *dest) {

    char *buf = src_path + strlen(watches->root_src);
    
    sprintf(dest, "%s%s", watches->root_tgt, buf);
}


void add_watches(AllWatches *allwatches, char *base_path){    
    
    add_watch(allwatches, base_path);
    DIR* dir;
    if((dir = opendir(base_path))==NULL)
    {
        ERR("opendir");
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char next_path[PATH_MAX];
            snprintf(next_path, PATH_MAX, "%s/%s", base_path, entry->d_name);
            add_watches(allwatches, next_path);
        }
    }
    closedir(dir);
}