#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lab_png.h"

void find_png(const char* path, int *png_cnt)
{
    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];
    char next_path[2048];
    struct stat buf;
    FILE *fp;
    
    if (lstat(path, &buf) < 0) {
        perror("lstat error");
        return;
    }
    if (S_ISREG(buf.st_mode)) {
        fp = fopen(path, "r");
        if (fp == NULL) {
            fprintf(stderr, "cannot open file %s\n", path);
            return;
        }
        if (is_png(fp)) {
            printf("%s\n", path);
            (*png_cnt)++;
        }
        fclose(fp);
        return;
    }
    if (S_ISDIR(buf.st_mode)) {
        if ((p_dir = opendir(path)) == NULL) {
            sprintf(str, "opendir(%s)", path);
            perror(str);
            return;
        }
        while ((p_dirent = readdir(p_dir)) != NULL) {
            char *str_path = p_dirent->d_name;  /* relative path name! */

            if (str_path == NULL) {
                fprintf(stderr, "Null pointer found!"); 
                continue;
            }
            if (strcmp(str_path, ".") == 0 || strcmp(str_path, "..") == 0)
                continue;
            snprintf(next_path, sizeof(next_path), "%s/%s", path, str_path);
            find_png(next_path, png_cnt);
        }
        if (closedir(p_dir) != 0) {
            perror("closedir");
            return;
        }
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s DIRECTORY\n", argv[0]);
        return 1;
    }
    int png_cnt = 0;
    find_png(argv[1], &png_cnt);
    if (png_cnt == 0) {
        printf("findpng: No PNG file found\n");
    }
    return 0;
}
