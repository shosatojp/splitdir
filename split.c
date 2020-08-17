#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <linux/limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct options {
    long total_size;
    long max_size;
    int page;
    char* pattern;
    char* base;
};

void walk(const char* dirname, int (*fn)(const char* path, struct dirent*, struct options*), struct options* opt) {
    DIR* dir = opendir(dirname);
    if (!dir) {
        printf("failed to open directory (%d): %s\n", errno, dirname);
        return;
    }

    struct dirent* dp;

    while (dp = readdir(dir)) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
            // ignore . ..
            continue;
            // } else if (*dp->d_name == '.') {
            //     // ignore dotfile
            //     continue;
        }

        // create fullpath
        char buf[PATH_MAX];
        sprintf(buf, "%s/%s", dirname, dp->d_name);

        switch (dp->d_type) {
            case 4:  // directory
                walk(buf, fn, opt);
                break;
            case 8:  // regular file
                fn(buf, dp, opt);
                break;
        }
    }

    closedir(dir);
}

FILE* cfd = NULL;

int statfile(const char* path, struct dirent* dp, struct options* opt) {
    static char out_path[PATH_MAX];
    struct stat sb;
    if (path && dp) {
        if (~stat(path, &sb)) {
            if (!cfd) {
                sprintf(out_path, opt->pattern, opt->page);
                cfd = fopen(out_path, "w");
            }
            if (opt->max_size - opt->total_size > sb.st_size) {
                opt->total_size += sb.st_size;
            } else {
                fclose(cfd);
                printf("page %d: %ld\n", opt->page, opt->total_size);
                opt->page++;
                opt->total_size = sb.st_size;
                sprintf(out_path, opt->pattern, opt->page);
                cfd = fopen(out_path, "w");
            }

            char buf[PATH_MAX + 3];
            for (char* b = opt->base; *b && *path && *b == *path; b++, path++)
                ;
            sprintf(buf, "%s\n", path);
            fwrite(buf, strlen(buf), 1, cfd);
        }
    } else {
        if (cfd)
            fclose(cfd);
        printf("page %d: %ld\n", opt->page, opt->total_size);
    }
}

long get_size(char* src) {
    int n = atoi(src);
    int len = (int)floor(log10(n)) + 1;

    char* unit = src + len;
    for (char* p = unit; *p; p++) *p = tolower(*p);

    long mul = 1;

    if (!strcmp(unit, "b"))
        mul = 1;
    else if (!strcmp(unit, "kb"))
        mul = 1000;
    else if (!strcmp(unit, "kib"))
        mul = 1024;
    else if (!strcmp(unit, "mb"))
        mul = 1000 * 1000;
    else if (!strcmp(unit, "mib"))
        mul = 1024 * 1024;
    else if (!strcmp(unit, "gb"))
        mul = 1000 * 1000 * 1000;
    else if (!strcmp(unit, "gib"))
        mul = 1024 * 1024 * 1024;

    return n * mul;
}

int main(int argc, char* argv[]) {
    struct options opt = {
        .total_size = 0,
        .max_size = 0,
        .page = 0,
        .pattern = "dirsplit.%d.txt",
        .base = "",
    };

    static struct option long_options[] = {
        {"base", required_argument, 0, 'b'},
        {"size", required_argument, 0, 's'},
        {"dir", required_argument, 0, 'd'},
        {0, 0, 0, 0},
    };

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "b:s:p:", long_options, &option_index);

        if (!~c) break;

        switch (c) {
            case 'b':
                opt.base = optarg;
                break;
            case 's':
                opt.max_size = get_size(optarg);
                break;
            case 'p':
                opt.pattern = optarg;
                break;
            default:
                exit(1);
                break;
        }
    }

    if (!opt.max_size) {
        printf("size (`-s`) required\n");
        exit(1);
    }

    if (optind < argc) {
        while (optind < argc) {
            char abspath[PATH_MAX];
            realpath(argv[optind++], abspath);
            walk(abspath, statfile, &opt);
        }
    }
    statfile(NULL, NULL, &opt);
}