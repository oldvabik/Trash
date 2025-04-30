#define _GNU_SOURCE
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

void init();

int find_first_slash(const char* s, int start_pos);
int find_last_slash(const char* s, int start_pos);

int my_unlink(const char* path_name, const char* func_name);
void compute_full_or_relative_path(char* old_path_name, const char* path_name, char* new_path);
void put_file_to_trash_with_checks(char* old_path_name, char* new_path, const char* func_name);

void logger(const char* new_path, const char* path_name, const char* function);
void println();

char cwd[MAX_PATH_LEN] = {0};
char home_path[MAX_PATH_LEN] = {0};
char trash_path[MAX_PATH_LEN] = {0};

int unlink(const char* path_name) {
    my_unlink(path_name, __func__);
    return 0;
}

int unlinkat(int dirfd, const char* path_name, int flags) {
    my_unlink(path_name, __func__);
    return 0;
}    

void init() {
    strcpy(home_path, getenv("HOME"));
    
    if (!*home_path) {
        fprintf(stderr, "ERROR: can't get home_path environment\n");
        exit(1);
    }

    strcpy(trash_path, home_path);
    strcat(trash_path, "/trash");
    getcwd(cwd, sizeof(cwd));  
    mkdir(trash_path, 0755);
}

int find_first_slash(const char* s, int start_pos) {
    int index = 0;

    for (int i = start_pos; s[i]; i++) {
        if (s[i] == '/') {
            index = i + 1;
            break;
        }
    }

    return index;
}

int find_last_slash(const char* s, int start_pos) {
    int index = 0;

    for (int i = start_pos - 1; i > 0 && s[i]; i--) {
        if (s[i] == '/') {
            index = i + 1;
            break;
        }
    }

    return index;
}  

int my_unlink(const char* path_name, const char* func_name) { 
    init();
    char old_path_name[MAX_PATH_LEN] = {0};
    char new_path[MAX_PATH_LEN] = {0};
    compute_full_or_relative_path(old_path_name, path_name, new_path);
    put_file_to_trash_with_checks(old_path_name, new_path, func_name);
    return 0;
}

void compute_full_or_relative_path(char* old_path_name, const char* path_name, char* new_path) {
    int index = 0;

    if (*path_name == '.' && *(path_name + 1) == '.') {
        char tmp_path_name[MAX_PATH_LEN] = {0};
        strcpy(tmp_path_name, cwd);
        index = find_last_slash(tmp_path_name, strlen(tmp_path_name));
        strncpy(old_path_name, tmp_path_name, index - 1);
        strcat(old_path_name, path_name + 2);
        index = find_last_slash(path_name, strlen(path_name));
    }
    else if (*path_name == '.' && *(path_name + 1) == '/') {
        strcpy(old_path_name, cwd);
        strcat(old_path_name, "/");
        strcat(old_path_name, path_name + 2);
        index = find_first_slash(path_name, 0);

    }
    else if (*path_name != '/') {
        strcpy(old_path_name, cwd);
        strcat(old_path_name, "/");
        strcat(old_path_name, path_name);
        index = find_first_slash(path_name, 0);
    }
    else {
        index = find_last_slash(path_name, strlen(path_name));
        strcpy(old_path_name, path_name);
    }

    strcpy(new_path, trash_path);
    strcat(new_path, "/");
    strcat(new_path, path_name + index);
}

void put_file_to_trash_with_checks(char* old_path_name, char* new_path, const char* func_name) {
    char tmp_path[MAX_PATH_LEN + 20] = {0};
    strcpy(tmp_path, new_path);
    FILE* file = fopen(new_path, "r");
    
    if (!file) {
        if (rename(old_path_name, new_path)) {
            fprintf(stderr, "file with this name doesn't exist\n");
            println();
            return;
        }   
    }
    else {
        int value = 0;
        do {
            value++;
            memset(tmp_path, 0, MAX_FILENAME_LEN);
            sprintf(tmp_path, "%s(%d)", new_path, value);
        } while (fopen(tmp_path, "r"));

        if (rename(old_path_name, tmp_path)) {
            fprintf(stderr, "file with this name doesn't exist\n");
            println();
            return;
        }   
        
        fclose(file);
    }

    logger(tmp_path, old_path_name, func_name);
}

void logger(const char *new_path, const char *path_name, const char *function) {
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    char log_path[MAX_PATH_LEN + 20];
    sprintf(log_path, "%s/trash.log", home_path);
    
    FILE *log = fopen(log_path, "a");  
    if (!log) {
        perror("fopen");
        exit(errno);
    }

    fprintf(log, "%s was renamed to %s by %s syscall on %s", 
    path_name, new_path, function, asctime(timeinfo));
	printf("%s was renamed to %s by %s syscall\n", 
    path_name, new_path, function);
    fclose(log);
}

void println() {
    printf("------------------------------------------------\n");
}
