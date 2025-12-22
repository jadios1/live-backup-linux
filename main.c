#include "common.h"
#include "file_ops.h"
#include "inotify.h"
#include "utils.h"

volatile sig_atomic_t keep_running = 1;

void handle_sig(int sig) { keep_running = 0; }

typedef struct
{
    pid_t pid;
    char source[PATH_MAX];
    char target[PATH_MAX];
} proc;

void add(char **argz, int wordcount, proc **processes, int *process_count)
{
    char source_path[PATH_MAX];
    char target_path[PATH_MAX];

    pid_t pid;

    char real_source_path[PATH_MAX];
    char real_target_path[PATH_MAX];

    strcpy(source_path, argz[0]);

    if (realpath(source_path, real_source_path) == NULL)
    {
        perror("cannot resolve source path");
        return;
    }

    for (int i = 1; i < wordcount; i++)
    {
        strcpy(target_path, argz[i]);

        if ((directory_checker(target_path, real_source_path)) == true)
        {
            if (realpath(target_path, real_target_path) == NULL)
            {
                perror("realpath target");
                continue;
            }

            bool exists = false;
            for (int k = 0; k < *process_count; k++)
            {
                if (strcmp((*processes)[k].target, real_target_path) == 0)
                {
                    exists = true;
                    break;
                }
            }
            if (exists)
                continue;

            if ((pid = fork()) < 0)
                ERR("fork");

            if (pid == 0)
            {
                copyfiles(real_source_path, real_target_path, real_source_path, real_target_path);
                AllWatches aw;
                aw.fd = inotify_init();
                aw.capacity = INITIAL_WATCH_CAPACITY;
                aw.count = 0;
                aw.watches = malloc(sizeof(Watch) * aw.capacity);
                strcpy(aw.root_src, real_source_path);
                strcpy(aw.root_tgt, real_target_path);

                add_watch_recursive(&aw, real_source_path);
                start_monitoring(&aw);
                exit(EXIT_SUCCESS);
            }
            else
            {
                (*process_count)++;

                proc *temp = realloc(*processes, sizeof(proc) * (*process_count));

                if (temp == NULL)
                    ERR("realloc");

                *processes = temp;

                int id = *process_count - 1;

                (*processes)[id].pid = pid;

                strncpy((*processes)[id].source, real_source_path, PATH_MAX);
                strncpy((*processes)[id].target, real_target_path, PATH_MAX);
            }
        }
    }
}

void usage_func() { fprintf(stderr, "commands : add, end, list, restore, exit.\n"); }

void end(char **argz, int wordcount, proc **processes, int *process_count)
{

    char real_target[PATH_MAX];
    char source_path[PATH_MAX];

    bool found = false;

    if (realpath(argz[0], source_path) == NULL)
    {
        perror("realpath");
        return;
    }

    for (int k = 1; k < wordcount; k++)
    {
        if (realpath(argz[k], real_target) == NULL)
        {
            perror("realpath");
            continue;
        }

        for (int i = 0; i < *process_count; i++)
        {
            if (strcmp((*processes)[i].target, real_target) == 0 && strcmp((*processes)[i].source, source_path) == 0)
            {
                kill((*processes)[i].pid, SIGTERM);
                waitpid((*processes)[i].pid, NULL, 0);

                printf("Ended backup for %s\n", real_target);
                (*processes)[i] = (*processes)[*process_count - 1];
                (*process_count)--;

                if (*process_count > 0)
                {
                    proc *temp = realloc(*processes, sizeof(proc) * (*process_count));
                    if (temp)
                        *processes = temp;
                }
                else
                {
                    free(*processes);
                    *processes = NULL;
                }
                found = true;
                break;
            }
        }
    }
    if (!found)
    {
        fprintf(stderr, "No backup process found");
    }
}

void list(proc *procs, int *proc_count)
{
    for (int i = 0; i < (*proc_count); i++)
    {
        printf("PROCESS:\n pid:%d\n source:%s\n target:%s\n", procs[i].pid, procs[i].source, procs[i].target);
    }
}

void exit_func() { keep_running = 0; }

void execute_command(char *buf, ssize_t nread, proc **processes, int *process_count)
{
    char *argz[64];
    int word = 0, character = 0;
    char arg[1024];
    bool quote = false;
    for (int i = 0; i < nread; i++)
    {
        if (buf[i] == '"')
        {
            if (quote == false)
                quote = true;
            else
                quote = false;
            continue;
        }
        if ((buf[i] == ' ' && quote == false) || buf[i] == '\n')
        {
            if (character > 0)
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
    if (word == 0)
        return;

    if (strcmp(argz[0], "add") == 0)
    {
        if (word >= 3)
        {
            add(argz + 1, word - 1, processes, process_count);
        }
        else
        {
            usage_func();
        }
    }
    else if (strcmp(argz[0], "end") == 0)
    {
        if (word >= 3)
        {
            end(argz + 1, word - 1, processes, process_count);
        }
        else
        {
            usage_func();
        }
    }
    else if (strcmp(argz[0], "restore") == 0)
    {
        fprintf(stderr, "i did not manage to implement restore ):\n");
    }
    else if (strcmp(argz[0], "list") == 0)
    {
        list(*processes, process_count);
    }
    else if (strcmp(argz[0], "exit") == 0)
    {
        for (int i = 0; i < word; i++)
        {
            free(argz[i]);
        }
        exit_func();
        return;
    }
    else
    {
        usage_func();
    }

    for (int i = 0; i < word; i++)
    {
        free(argz[i]);
    }
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_sig;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1)
        perror("sigaction SIGINT");
    if (sigaction(SIGTERM, &sa, NULL) == -1)
        perror("sigaction SIGTERM");

    int process_count = 0;
    proc *processes = NULL;
    char *buf = NULL;
    size_t len = 0;

    printf("commands : add, end, list, restore, exit.\n");

    while (keep_running)
    {
        printf("> ");
        fflush(stdout);

        errno = 0;
        ssize_t nread = getline(&buf, &len, stdin);

        if (nread == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }

        execute_command(buf, nread, &processes, &process_count);
    }

    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid > 0)
        {
            kill(processes[i].pid, SIGTERM);
            waitpid(processes[i].pid, NULL, 0);
        }
    }

    free(processes);
    if (buf)
    {
        free(buf);
    }
    return 0;
}
