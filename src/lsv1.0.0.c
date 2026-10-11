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

extern int errno;

enum DisplayMode {
    MODE_VERTICAL,
    MODE_LONG,
    MODE_HORIZONTAL
};

enum DisplayMode display_mode = MODE_VERTICAL;

int get_terminal_width(void);
void mode_to_letters(int mode, char str[]);
void print_long_entry(const char *dir, const char *name);
void do_ls(const char *dir);
void print_down_then_across(char **names, int count, int term_width, int max_len);
void print_horizontal(char **names, int count, int term_width, int max_len);


int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            display_mode = MODE_LONG;
            break;
        case 'x':
            display_mode = MODE_HORIZONTAL;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
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
            if (argc - optind > 1)
            {
                printf("Directory listing of %s : \n", argv[i]);
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
        print_horizontal(names, count, term_width, max_len);
    }
    // Feature 3: Down-then-across column display
    else
    {
        int term_width = get_terminal_width();
        print_down_then_across(names, count, term_width, max_len);
    }

    // Free memory
    for (int i = 0; i < count; i++)
    {
        free(names[i]);
    }
    free(names);
}


void print_down_then_across(char **names, int count, int term_width, int max_len) {
    int spacing = 2;
    int col_width = max_len + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            int idx = c * num_rows + r;
            if (idx < count) {
                if (c == num_cols - 1 || idx + num_rows >= count) {
                    printf("%s", names[idx]);
                } else {
                    printf("%-*s", col_width, names[idx]);
                }
            }
        }
        printf("\n");
    }
}

void print_horizontal(char **names, int count, int term_width, int max_len) {
    int col_width = max_len + 2;
    int cur_x = 0;
    for (int i = 0; i < count; i++) {
        if (cur_x + col_width > term_width && cur_x > 0) {
            printf("\n");
            cur_x = 0;
        }
        printf("%-*s", col_width, names[i]);
        cur_x += col_width;
    }
    if (cur_x > 0) printf("\n");
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

    printf("%s %2ld %-8s %-8s %8ld %s %s\n",
           modestr, (long)st.st_nlink, uname, gname,
           (long)st.st_size, date_buf, name);
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