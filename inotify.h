#include "common.h"
#define INITIAL_WATCH_CAPACITY 16
#define WATCH_GROWTH_FACTOR 2


typedef struct {
    int wd;
    char path[PATH_MAX];
} Watch;

typedef struct {
    int fd;
    Watch *watches;
    int count;
    int capacity;
    char root_src[PATH_MAX];
    char root_tgt[PATH_MAX];
} AllWatches;

void add_watch(AllWatches *allwatches,char *path);


void add_watches(AllWatches *allwatches, char *base_path);