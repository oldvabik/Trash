#define _GNU_SOURCE
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// Цветовые коды ANSI
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_GRAY    "\033[1;90m"
#define COLOR_WHITE   "\033[1;97m"

void init();
void input_option(char* option);
void println();
void print_menu();
void compute_paths();

int check_trash_log();

void list_trash_files();

void input_full_or_relative_file_path(char* old_path_name, char* path_name, char* new_path);
void put_file_to_trash_with_checks(char* old_path_name, char* new_path);

void input_file_path(char* new_path);
void restore_file(const char* filename);

void delete_file_permanently(const char* filename);
void delete_line_from_trashlog_file(const char* dest);

void clear_trashbin();
void clear_trash_log();

int find_first_slash(const char *s, int start_pos);
int find_last_slash(const char *s, int start_pos);

void logger(const char *path_name, const char *new_path);

char cwd[MAX_PATH_LEN] = {0};
char home_path[MAX_PATH_LEN] = {0};
char trash_log_path[MAX_PATH_LEN] = {0};
char trash_path[MAX_PATH_LEN + 10] = {0};

int main() {
    init();
    char option;
    while (true) {
        input_option(&option);
        switch (option) {
            case 'l':
            {
                list_trash_files();
                break;
            }
            case 'p':
            {
                char old_path_name[MAX_PATH_LEN] = {0};
                char path_name[MAX_FILENAME_LEN] = {0};
                char new_path[MAX_PATH_LEN] = {0};
                input_full_or_relative_file_path(old_path_name, path_name, new_path);
                put_file_to_trash_with_checks(old_path_name, new_path);
                println();
                break;
            }
            case 'r':
            {
                if (check_trash_log() == -1) {
                    break;
                }
                char new_path[MAX_PATH_LEN] = {0};
                printf(COLOR_CYAN "Input name of file you want to restore: " COLOR_RESET);
                input_file_path(new_path);
                restore_file(new_path);
                println();
                break;
            }
            case 'd':
            {
                if (check_trash_log() == -1) {
                    break;
                }
                char path_name[MAX_PATH_LEN];
                printf(COLOR_CYAN "Input name of file you want to delete permanently: " COLOR_RESET);
                input_file_path(path_name);
                delete_file_permanently(path_name);
                break;
            }
            case 'c':
            {
                if(check_trash_log() == -1) break;
                clear_trash_log();
                clear_trashbin();
                printf(COLOR_GREEN "Trashbin was successfully cleared\n" COLOR_RESET);
                println();
                break;
            }
            case 'm':
            {
                print_menu();
                break;
            }
            case 'q':
            {
                exit(0);
                break;
            }
        }
    }
    return 0;
}

void init() {
    println();
    print_menu();
    compute_paths();
    mkdir(trash_path, 0755);
}

void input_option(char* option) {
    char tmp;
    printf(COLOR_CYAN "Select option: [l/p/r/d/c/m/q]: " COLOR_RESET);
    fflush(stdout);
    
    while (true) {
        scanf("%c", &tmp);
        if (tmp == '\n') continue;
        if (tmp == 'l' || tmp == 'p' || tmp == 'r' || tmp == 'd' || 
            tmp == 'c' || tmp == 'm' || tmp == 'q') {
            break;
        }
        printf(COLOR_RED "Invalid option! Please choose [l/p/r/d/c/m/q]: " COLOR_RESET);
        while(getchar() != '\n');
    }
    *option = tmp;
    println();
}

void println() {
    printf(COLOR_GRAY "----------------------------------------------------------------------\n" COLOR_RESET);
}

void print_menu() {
    printf(COLOR_MAGENTA "\t\t...---=== Trash Manager ===---...\n" COLOR_RESET);
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "l");
    printf(COLOR_CYAN "] - List of trash files\t");
    
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "p");
    printf(COLOR_CYAN "] - Put the file in the trash\n");
    
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "r");
    printf(COLOR_CYAN "] - Restore file from trash\t");
    
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "d");
    printf(COLOR_CYAN "] - Delete file from trash permanently\n");
    
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "c");
    printf(COLOR_CYAN "] - Clear trash\t\t");
    
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "m");
    printf(COLOR_CYAN "] - Print menu\n");
    
    printf(COLOR_CYAN "[");
    printf(COLOR_YELLOW "q");
    printf(COLOR_CYAN "] - Quit\n" COLOR_RESET);
    
    println();
}

void compute_paths() {
    strcpy(home_path, getenv("HOME"));

    if (!(*home_path)) {
        fprintf(stderr, COLOR_RED "ERROR: can't get home_path environment\n" COLOR_RESET);
        exit(1);
    }

    sprintf(trash_path, "%s/trash", home_path);
    strcpy(trash_log_path, home_path);
    strcat(trash_log_path, "/trash.log");  
    getcwd(cwd, sizeof(cwd));  
}

int check_trash_log() {
    int fd = open(trash_log_path, O_RDWR);
    if (!fd) {
        perror(COLOR_RED "open" COLOR_RESET);
        exit(errno);
    }

    struct stat s;
    fstat(fd, &s);
    close(fd);

    if (!s.st_size) {
        fprintf(stderr, COLOR_YELLOW "Trash is empty, put files to it first\n" COLOR_RESET);
        println();
        return -1;
    }

    return 0;
}

void list_trash_files() {
    DIR* dir = opendir(trash_path);
    struct dirent* read_dir;

    if (!dir && errno) {
        perror(COLOR_RED "opendir" COLOR_RESET);
        errno = 0;
        return;
    }

    int files_count = 0;
    while ((read_dir = readdir(dir))) {
        if (!(strcmp(read_dir->d_name, ".") && strcmp(read_dir->d_name, ".."))) {
            continue;
        }
        
        if (!files_count) {
            printf(COLOR_CYAN "Trash files:\n" COLOR_RESET);
        }

        printf(COLOR_WHITE "%s\n" COLOR_RESET, read_dir->d_name);
        files_count++;
    }

    if (!files_count) {
        fprintf(stderr, COLOR_YELLOW "Trash is empty\n" COLOR_RESET);
    }

    println();
    closedir(dir);
}

void input_full_or_relative_file_path(char* old_path_name, char* path_name, char* new_path) {
    printf(COLOR_CYAN "Input name of file you want to put into trashbin: " COLOR_RESET);
    fflush(stdin);
    fgets(path_name, MAX_FILENAME_LEN, stdin);
    fgets(path_name, MAX_FILENAME_LEN, stdin);
    path_name[strlen(path_name) - 1] = '\0';

    int index = 0;
    if (*path_name == '.' && *(path_name + 1) == '.') {
        char tmp_path_name[MAX_PATH_LEN];
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
    else if(*path_name != '/') {
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

void put_file_to_trash_with_checks(char* old_path_name, char* new_path) {
    char tmp_path[MAX_PATH_LEN + 20];
    strcpy(tmp_path, new_path);

    FILE* file = fopen(new_path, "r");

    if (!file) {
        if (rename(old_path_name, new_path)) {
            fprintf(stderr, COLOR_RED "File with this name doesn't exist\n" COLOR_RESET);
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
            fprintf(stderr, COLOR_RED "File with this name doesn't exist\n" COLOR_RESET);
            return;
        }

        fclose(file);
    }

    logger(old_path_name, tmp_path);
}

void input_file_path(char* new_path) {
    char path_name[MAX_FILENAME_LEN];
    fflush(stdin);
    fgets(path_name, sizeof(path_name)/sizeof(*path_name), stdin);
    fgets(path_name, sizeof(path_name)/sizeof(*path_name), stdin);
    path_name[strlen(path_name) - 1] = '\0';

    strcpy(new_path, trash_path);
    strcat(new_path, "/");
    strcat(new_path, path_name);
}

void restore_file(const char* filename) {
    FILE* file = fopen(trash_log_path, "r+");

    if (!file) {
        perror(COLOR_RED "fopen" COLOR_RESET);
        exit(errno);
    }

    fseek(file, 0, SEEK_SET);
    char file_path[MAX_PATH_LEN];
    char dest[MAX_PATH_LEN];   

    while (!feof(file)) {
        memset(file_path, 0, MAX_PATH_LEN);
        memset(dest, 0, MAX_PATH_LEN);
        
        if (!fgets(file_path, MAX_PATH_LEN, file)) {
            break;
        }
        
        char* path_end = strstr(file_path, " was renamed to ");

        if (path_end) {
            strncpy(dest, file_path, ((path_end - file_path)/sizeof(char)));
        }

        int dest_after_index = find_last_slash(dest, strlen(dest));
        int filename_index = find_last_slash(filename, strlen(filename));

        if (strcmp(dest + dest_after_index, filename + filename_index)) {
            int last_idx = find_last_slash(file_path, strlen(file_path));
            int tmp = last_idx;
            int count = 0;

            while (*(file_path + tmp) != ' ') {
                tmp++;
                count++;
            }

            if (strncmp(filename + filename_index, file_path + last_idx, count)) {
                continue;
            }
        }

        fclose(file);
        FILE* end = fopen(dest, "r"); 
        char tmp_path[MAX_PATH_LEN + 20];

        if (!end) {
            if(rename(filename, dest)) {
                perror(COLOR_RED "rename" COLOR_RESET);
                println();
                break;
            } 

            printf(COLOR_GREEN "%s" COLOR_RESET " was restored from trashbin and renamed to " COLOR_YELLOW "%s\n" COLOR_RESET, 
            filename, dest);
        }
        else {
            int value = 0;
            do {
                value++;
                memset(tmp_path, 0, MAX_FILENAME_LEN);
                sprintf(tmp_path, "%s(%d)", dest, value);
            } while (fopen(tmp_path, "r"));

            if (rename(filename, tmp_path)) {
                perror(COLOR_RED "rename" COLOR_RESET);
                println();
                break;
            }

            printf(COLOR_GREEN "%s was restored from trashbin and renamed to %s because of collisions\n" COLOR_RESET, filename, tmp_path);
            fclose(end);
        }
        
        delete_line_from_trashlog_file(dest);
        return;
    }

    fclose(file);
    printf(COLOR_YELLOW "There is no such file in trashbin\n" COLOR_RESET);
}

void delete_file_permanently(const char* filename) {
    FILE* file = fopen(trash_log_path, "r+");

    if (!file) {
        perror(COLOR_RED "fopen" COLOR_RESET);
        exit(errno);
    }

    fseek(file, 0, SEEK_SET);
    char file_path[MAX_PATH_LEN];
    char dest[MAX_PATH_LEN];
    
    while (!feof(file)) {
        memset(file_path, 0, MAX_PATH_LEN);
        memset(dest, 0, MAX_PATH_LEN);
        
        if (!fgets(file_path, MAX_PATH_LEN, file)) {
            break;
        }
        
        char *path_end = strstr(file_path, " was renamed to ");
        
        if (path_end) {
            strncpy(dest, file_path, ((path_end - file_path)/sizeof(char)));
        }

        int dest_after_index = find_last_slash(dest, strlen(dest));
        int filename_index = find_last_slash(filename, strlen(filename));
        
        if (strcmp(dest + dest_after_index, filename + filename_index)) {
            continue;
        }

        if (remove(filename)) {
            perror(COLOR_RED "remove" COLOR_RESET);
            exit(errno);  
        }

        printf(COLOR_GREEN "%s was deleted permanently from trashbin\n" COLOR_RESET, filename);
        println();
        delete_line_from_trashlog_file(dest);
        fclose(file);
        return;
    }

    fclose(file);
    printf(COLOR_YELLOW "There is no such file in trashbin\n" COLOR_RESET);
    println();
}

void delete_line_from_trashlog_file(const char* dest) {
    int fd = open(trash_log_path, O_RDWR);

    if (!fd) {
        perror("open");
        exit(errno);
    }

    struct stat s;
    fstat(fd, &s);
    char *file_text = (char*) mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    char *line_begin = strstr(file_text, dest);
    char *line_end = line_begin;
    
    while (*line_end != '\n' && line_end) {
        line_end++;
    }

    if (*line_end) {
        line_end++;
    }

    int new_file_size = s.st_size - (line_end - line_begin);
    
    while (line_end < file_text + s.st_size) {
        *line_begin = *line_end;
        line_begin++;
        line_end++;
    }

    ftruncate(fd, new_file_size);
    munmap(file_text, s.st_size);
    close(fd);
}

void clear_trashbin() {
    struct dirent *read_dir;
    DIR *dir = opendir(trash_path);

    if (!dir && errno) {
        perror("opendir");
        errno = 0;
        return;
    }

    while ((read_dir = readdir(dir))) {
        if (!(strcmp(read_dir->d_name, ".") && strcmp(read_dir->d_name, ".."))) {
            continue;
        }

        char file_path[MAX_PATH_LEN];
        strcpy(file_path, trash_path);
        strcat(file_path, "/");
        strcat(file_path, read_dir->d_name);
        
        if (remove(file_path)) {
            perror("remove");
        }
    }

    closedir(dir);
}

void clear_trash_log() {
    FILE *file = fopen(trash_log_path, "w");
    
    if (!file) {
        perror("fopen");
        exit(errno);
    }

    fclose(file);
}

int find_first_slash(const char* s, int start_pos) {
    int index = 0;

    for (int i = start_pos; s[i]; i++) {
        if(s[i] == '/') {
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

void logger(const char* path_name, const char* new_path) {
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    FILE *log = fopen(trash_log_path, "a");  
    
    if (!log) {
        perror(COLOR_RED "fopen" COLOR_RESET);
        exit(errno);
    }

    fseek(log, 0, SEEK_END);         
    fprintf(log, "%s was renamed to %s by user on %s", path_name, new_path, 
    asctime(timeinfo));
    
    printf(COLOR_GREEN "%s" COLOR_RESET " was renamed to " COLOR_YELLOW "%s" COLOR_RESET " by user\n", 
           path_name, new_path);
    
    fclose(log);
}