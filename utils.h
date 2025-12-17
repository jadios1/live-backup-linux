#include "common.h"

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))


bool directory_checker(char *target_path, char *real_source);

