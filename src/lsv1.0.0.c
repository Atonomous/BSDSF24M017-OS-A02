/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_PINK    "\033[1;35m"
#define COLOR_REVERSE "\033[7m"

extern int errno;

enum DisplayMode {
    MODE_VERTICAL,
    MODE_LONG,
    MODE_HORIZONTAL
};

enum DisplayMode display_mode = MODE_VERTICAL;
int flag_R = 0;

int get_terminal_width(void);
void mode_to_letters(int mode, char str[]);
void print_long_entry(const char *dir, const char *name);
void do_ls(const char *dir);
void print_down_then_across(const char *dir, char **names, int count, int term_width, int max_len);
void print_horizontal(const char *dir, char **names, int count, int term_width, int max_len);
int compare_filenames(const void *a, const void *b);
int is_archive(const char *name);
void print_colored_name(const char *dir, const char *name);


int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        switch (opt)
        {
        case 'l':
            display_mode = MODE_LONG;
            break;
        case 'x':
            display_mode = MODE_HORIZONTAL;
            break;
        case 'R':
            flag_R = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1 && !flag_R)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i]);
            if (i < argc - 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

void do_ls(const char *dir)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    if (flag_R)
    {
        printf("%s:\n", dir);
    }
    int capacity = 64;
    int count = 0;
    char **names = malloc(capacity * sizeof(char *));
    int max_len = 0;

    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity)
        {
            capacity *= 2;
            names = realloc(names, capacity * sizeof(char *));
        }

        names[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;
        count++;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }
    closedir(dp);

    // Sort entries alphabetically
    qsort(names, count, sizeof(char *), compare_filenames);

    // Feature 2: -l long listing
    if (display_mode == MODE_LONG)
    {
        for (int i = 0; i < count; i++)
        {
            print_long_entry(dir, names[i]);
        }
    }
    // Feature 4: -x horizontal listing
    else if (display_mode == MODE_HORIZONTAL)
    {
        int term_width = get_terminal_width();
        print_horizontal(dir, names, count, term_width, max_len);
    }
    // Feature 3: Down-then-across column display
    else
    {
        int term_width = get_terminal_width();
        print_down_then_across(dir, names, count, term_width, max_len);
    }

    // Recursive traversal if -R enabled
    if (flag_R)
    {
        for (int i = 0; i < count; i++)
        {
            char subpath[1024];
            if (strcmp(dir, "/") == 0)
                snprintf(subpath, sizeof(subpath), "/%s", names[i]);
            else
                snprintf(subpath, sizeof(subpath), "%s/%s", dir, names[i]);

            struct stat st;
            if (lstat(subpath, &st) == 0 && S_ISDIR(st.st_mode))
            {
                putchar('\n');
                do_ls(subpath);
            }
        }
    }

    // Free memory
    for (int i = 0; i < count; i++)
    {
        free(names[i]);
    }
    free(names);
}


void print_down_then_across(const char *dir, char **names, int count, int term_width, int max_len) {
    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = c * num_rows + r;
            if (idx < count) {
                print_colored_name(dir, names[idx]);
                if (c < num_cols - 1 && idx + num_rows < count) {
                    int pad = col_width - strlen(names[idx]);
                    for (int s = 0; s < pad; s++) putchar(' ');
                }
            }
        }
        putchar('\n');
    }
}

void print_horizontal(const char *dir, char **names, int count, int term_width, int max_len) {
    int col_width = max_len + 2;
    int cur_x = 0;
    for (int i = 0; i < count; i++) {
        if (cur_x + col_width > term_width && cur_x > 0) {
            putchar('\n');
            cur_x = 0;
        }
        print_colored_name(dir, names[i]);
        int pad = col_width - strlen(names[i]);
        for (int s = 0; s < pad; s++) putchar(' ');
        cur_x += col_width;
    }
    if (cur_x > 0) putchar('\n');
}


int compare_filenames(const void *a, const void *b) {
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

int is_archive(const char *name) {
    int len = strlen(name);
    if (len >= 4 && (strcmp(name + len - 4, ".tar") == 0 || strcmp(name + len - 4, ".zip") == 0)) return 1;
    if (len >= 3 && strcmp(name + len - 3, ".gz") == 0) return 1;
    return 0;
}

void print_colored_name(const char *dir, const char *name) {
    char fullpath[1024];
    if (strcmp(dir, "/") == 0) snprintf(fullpath, sizeof(fullpath), "/%s", name);
    else snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    struct stat st;
    if (lstat(fullpath, &st) == -1) {
        printf("%s", name);
        return;
    }

    if (S_ISLNK(st.st_mode))
        printf("%s%s%s", COLOR_PINK, name, COLOR_RESET);
    else if (S_ISDIR(st.st_mode))
        printf("%s%s%s", COLOR_BLUE, name, COLOR_RESET);
    else if (is_archive(name))
        printf("%s%s%s", COLOR_RED, name, COLOR_RESET);
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        printf("%s%s%s", COLOR_GREEN, name, COLOR_RESET);
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
        printf("%s%s%s", COLOR_REVERSE, name, COLOR_RESET);
    else
        printf("%s", name);
}

void print_long_entry(const char *dir, const char *name) {
    char fullpath[1024];
    if (strcmp(dir, "/") == 0) snprintf(fullpath, sizeof(fullpath), "/%s", name);
    else snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    struct stat st;
    if (lstat(fullpath, &st) == -1) {
        perror("lstat failed");
        return;
    }

    char modestr[11];
    mode_to_letters(st.st_mode, modestr);

    struct passwd *pw = getpwuid(st.st_uid);
    char uname[32];
    if (pw) snprintf(uname, sizeof(uname), "%s", pw->pw_name);
    else snprintf(uname, sizeof(uname), "%d", st.st_uid);

    struct group *gr = getgrgid(st.st_gid);
    char gname[32];
    if (gr) snprintf(gname, sizeof(gname), "%s", gr->gr_name);
    else snprintf(gname, sizeof(gname), "%d", st.st_gid);

    char *time_str = ctime(&st.st_mtime);
    char date_buf[13];
    if (time_str) {
        strncpy(date_buf, time_str + 4, 12);
        date_buf[12] = '\0';
    } else {
        strcpy(date_buf, "Unknown date");
    }

    printf("%s %2ld %-8s %-8s %8ld %s ",
           modestr, (long)st.st_nlink, uname, gname,
           (long)st.st_size, date_buf);
    print_colored_name(dir, name);
    putchar('\n');
}



void mode_to_letters(int mode, char str[]) {
    strcpy(str, "----------");
    // File type
    if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else str[0] = '-';

    // Permissions
    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';
}

int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return 80;
}
