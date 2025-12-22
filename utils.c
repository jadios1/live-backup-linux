#include "utils.h"



bool directory_checker(char *target_path, char *real_source) {
    
    char real_target[PATH_MAX];

    if (realpath(target_path, real_target) == NULL) {
        if (mkdir(target_path, 0777) != 0) {
            perror("mkdir");
            return false;
        }
        if (realpath(target_path, real_target) == NULL) {
            perror("realpath for target_path");
            return false;
        }
    }
    if (strcmp(real_source, real_target) == 0) {
        fprintf(stderr, "Target cannot be the same as source: %s\n", target_path);
        return false;
    }
    size_t source_len = strlen(real_source);
    if (strncmp(real_source, real_target, source_len) == 0 &&
        (real_target[source_len] == '/' || real_target[source_len] == '\0')) {
        fprintf(stderr, "Target cannot be inside source directory: %s\n", target_path);
        return false;
    }

    struct stat st;
    if (stat(real_target, &st) != 0) {
        perror("stat for target_path");
        return false;
    }

    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Target exists but is not a directory: %s\n", target_path);
        return false;
    }

    DIR *dir = opendir(real_target);
    if (!dir) {
        perror("opendir realpath");
        return false;
    }

    struct dirent *entry;
    bool empty = true;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        empty = false;
        break;
    }

    closedir(dir);

    if (!empty) {
        fprintf(stderr, "Target directory is not empty: %s\n", target_path);
        return false;
    }

    return true;
}

