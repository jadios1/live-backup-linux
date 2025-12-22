#include "common.h"
#define INITIAL_WATCH_CAPACITY 16

typedef struct
{
    int wd;
    char *path;
} Watch;

typedef struct
{
    int fd;
    Watch *watches;
    int count;
    int capacity;
    char root_src[PATH_MAX];
    char root_tgt[PATH_MAX];
} AllWatches;

char *get_path_from_wd(AllWatches *aw, int wd);

void translate_path(const char *src_full, AllWatches *aw, char *target_full);

void remove_from_map(AllWatches *aw, int wd);

void add_watch_recursive(AllWatches *aw, char *base_path);

void update_watch_paths(AllWatches *aw, const char *old_path, const char *new_path);

void start_monitoring(AllWatches *aw);
