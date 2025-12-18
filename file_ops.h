#include "common.h"

void copyfile(char* source_file_path, char* target_file_path,struct stat stat);

void copyfiles(char* source_path,char* target_path,char* root_source_path, char* root_target_path);

void copysymlink(char* source_file_path, char* target_file_path, char* root_source_path, char* root_target_path);