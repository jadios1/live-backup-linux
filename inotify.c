#include "inotify.h"
#include "file_ops.h"
#include "utils.h"

#define EVENT_BUF_LEN (64 * (sizeof(struct inotify_event) + NAME_MAX + 1))

volatile sig_atomic_t backup_running = 1;

void handle_backup_sig(int sig) { backup_running = 0; }

char *get_path_from_wd(AllWatches *aw, int wd)
{
    for (int i = 0; i < aw->count; i++)
    {
        if (aw->watches[i].wd == wd)
            return aw->watches[i].path;
    }
    return NULL;
}

void translate_path(const char *src_full, AllWatches *aw, char *target_full)
{
    size_t root_len = strlen(aw->root_src);
    const char *suffix = src_full + root_len;
    sprintf(target_full, "%s%s", aw->root_tgt, suffix);
}

void remove_from_map(AllWatches *aw, int wd)
{
    for (int i = 0; i < aw->count; i++)
    {
        if (aw->watches[i].wd == wd)
        {
            free(aw->watches[i].path);
            aw->watches[i] = aw->watches[aw->count - 1];
            aw->count--;

            return;
        }
    }
}

void add_watch_recursive(AllWatches *aw, char *base_path)
{
    if (aw->count >= aw->capacity)
    {
        Watch *temp = realloc(aw->watches, sizeof(Watch) * aw->capacity * 2);
        if (temp != NULL)
        {
            aw->watches = temp;
            aw->capacity = aw->capacity * 2;
        }
        else
        {
            perror("realloc when capacity full");
            return;
        }
    }

    uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_CLOSE_WRITE |
                    IN_DELETE_SELF | IN_MOVE_SELF;

    int wd = inotify_add_watch(aw->fd, base_path, mask);
    if (wd < 0)
    {
        return;
    }

    aw->watches[aw->count].wd = wd;
    aw->watches[aw->count].path = strdup(base_path);
    aw->count++;

    DIR *dir = opendir(base_path);
    if (!dir)
    {
        perror("opendir error");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", base_path, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            add_watch_recursive(aw, full_path);
        }
    }
    closedir(dir);
}

void update_watch_paths(AllWatches *aw, const char *old_path, const char *new_path)
{
    size_t old_len = strlen(old_path);

    for (int i = 0; i < aw->count; i++)
    {
        if (strncmp(aw->watches[i].path, old_path, old_len) == 0 &&
            (aw->watches[i].path[old_len] == '/' || aw->watches[i].path[old_len] == '\0'))
        {
            char new_full_path[PATH_MAX];
            snprintf(new_full_path, PATH_MAX, "%s%s", new_path, aw->watches[i].path + old_len);

            free(aw->watches[i].path);
            aw->watches[i].path = strdup(new_full_path);
        }
    }
}

void start_monitoring(AllWatches *aw)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_backup_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    uint32_t pending_cookie = 0;
    char pending_move_path[PATH_MAX] = "";

    char buffer[EVENT_BUF_LEN];

    while (aw->count > 0 && backup_running)
    {
        ssize_t len = read(aw->fd, buffer, EVENT_BUF_LEN);

        if (len < 0)
        {
            if (errno == EINTR)
            {
                if (!backup_running)
                    break;

                continue;
            }
            perror("read");
            break;
        }
        if (len == 0)
            break;

        ssize_t i = 0;

        while (i < len && backup_running)
        {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];

            if (event->mask & IN_IGNORED)
            {
                remove_from_map(aw, event->wd);
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }
            if (event->mask & IN_DELETE_SELF || event->mask & IN_MOVE_SELF)
            {
                char *path = get_path_from_wd(aw, event->wd);
                if (path && strcmp(path, aw->root_src) == 0)
                {
                    backup_running = 0;
                    break;
                }
            }

            if (event->len == 0)
            {
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }

            char *parent_path = get_path_from_wd(aw, event->wd);
            if (!parent_path)
            {
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }

            char src_full[PATH_MAX];
            char tgt_full[PATH_MAX];
            snprintf(src_full, PATH_MAX, "%s/%s", parent_path, event->name);
            translate_path(src_full, aw, tgt_full);

            if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM)
            {
                if ((event->mask & IN_MOVED_FROM) && (event->mask & IN_ISDIR))
                {
                    pending_cookie = event->cookie;
                    strncpy(pending_move_path, src_full, PATH_MAX);
                }
                remove_recursive(tgt_full);
            }

            else if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO)
            {
                if ((event->mask & IN_MOVED_TO) && (event->mask & IN_ISDIR))
                {
                    if (event->cookie == pending_cookie && pending_cookie != 0)
                    {
                        update_watch_paths(aw, pending_move_path, src_full);
                        pending_cookie = 0;
                    }
                }

                struct stat st;
                if (lstat(src_full, &st) == 0)
                {
                    if (S_ISDIR(st.st_mode))
                    {
                        mkdir(tgt_full, 0777);
                        add_watch_recursive(aw, src_full);
                        copyfiles(src_full, tgt_full, aw->root_src, aw->root_tgt);
                    }
                    else if (S_ISLNK(st.st_mode))
                    {
                        copysymlink(src_full, tgt_full, aw->root_src, aw->root_tgt);
                    }
                    else if (S_ISREG(st.st_mode))
                    {
                        copyfile(src_full, tgt_full, st);
                    }
                }
            }

            else if (event->mask & IN_MODIFY || event->mask & IN_CLOSE_WRITE || event->mask & IN_ATTRIB)
            {
                struct stat st;
                if (lstat(src_full, &st) == 0 && S_ISREG(st.st_mode))
                {
                    copyfile(src_full, tgt_full, st);
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    for (int k = 0; k < aw->count; k++)
    {
        free(aw->watches[k].path);
    }
    free(aw->watches);
}
