#include <stdio.h>
#include <pwd.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

typedef struct psinfo {
    int pid;
    char name[65];
    unsigned long vsize;
    unsigned long cputime;
    char user[128];
} psinfo;

int psinfo_cmp_cputime(const void * a, const void * b) {
    // descending order
    long long a_ = (*(psinfo**)a)->cputime;
    long long b_ = (*(psinfo**)b)->cputime;
    return b_ - a_;
}

int psinfo_cmp_vsize(const void * a, const void * b) {
    // descending order
    long long a_ = (*(psinfo**)a)->vsize;
    long long b_ = (*(psinfo**)b)->vsize;
    return b_ - a_;
}

void print_ps(psinfo* ps) {
    int hour = ps->cputime / 3600;
    ps->cputime -= hour * 3600;
    int minute = ps->cputime / 60;
    ps->cputime -= minute * 60;
    int second = ps->cputime;

    printf("%7d %-20s %02d:%02d:%02d %10lu %s\n", ps->pid, ps->user, hour, minute, second, ps->vsize, ps->name);
}

void print_all_ps(psinfo* pses[], int count){
    printf("%7s %-20s %8s %10s %s\n", "PID", "USER", "TIME", "VIRT", "CMD");
    for (int i = 0; i < count; i++) {
        psinfo* ps = pses[i];
        print_ps(ps);
    }
}

int main(int argc, char* args[]) {
    psinfo* pses[1024];
    int count = 0;
    int sort = 0, filter = 0;
    char user[65];

    int c;
    while ((c = getopt(argc, args, "mpu:")) != -1) {
        switch (c)
        {
            case 'm':
                sort = 1;
                break;
            case 'p':
                sort = 2;
                break;
            case 'u':
                strcpy(user, optarg);
                filter = 1;
                break;
        }
    }

    DIR* proc = opendir("/proc");
    struct dirent *entry;
    while ((entry = readdir(proc)) != NULL) {
        int pid;
        if (sscanf(entry->d_name, "%d", &pid) == 0) continue;

        char name[65];
        unsigned long vsize;
        unsigned long utime, stime;

        char status_str[64];
        sprintf(status_str, "/proc/%d/status", pid);
        FILE* status_fp = fopen(status_str, "r");
        uid_t uid;
        char line[1024];
        while (line != NULL) {
            fgets(line, 1024, status_fp);
            char* token = strtok(line, "\t");
            if (strcmp(token, "Uid:") == 0) {
                token = strtok(NULL, "\t");
                token = strtok(NULL, "\t"); // effective UID
                uid = strtoul(token, NULL, 10);
                break;
            }
        }
        fclose(status_fp);
        if (filter == 1 && strcmp(user, getpwuid(uid)->pw_name) != 0) continue;

        char stat_str[64];
        sprintf(stat_str, "/proc/%d/stat", pid);
        FILE* stat_fp = fopen(stat_str, "r");
        char data[2048];
        fgets(data, 2048, stat_fp);
        // char status;
        // sscanf(data, "%*d %*s %c ", &status);
        // if (status != 'R' && status != 'S') continue;
        // printf("%s\n", data);
        char *token = strtok(data, " ");
        for (int i = 0; token != NULL ; i++) {
            if (i == 1) {
                int len = strlen(token);
                strncpy(name, token+1, len-2);
                name[len-2] = '\0';
            } else if (i == 13) {
                utime = strtoul(token, NULL, 10);
            } else if (i == 14) {
                stime = strtoul(token, NULL, 10);
            } else if (i == 22) {
                vsize = strtoul(token, NULL, 10);
                break;
            }
            token = strtok(NULL, " ");
        }
        // printf("%d %lu %lu, %lu %s\n", pid, vsize, utime, stime, name);
        fclose(stat_fp);


        psinfo* ps = (psinfo*)malloc(sizeof(psinfo));
        ps->pid = pid;
        strcpy(ps->name, name);
        ps->cputime = (utime + stime)/sysconf(_SC_CLK_TCK);
        ps->vsize = vsize/1000;
        strcpy(ps->user, getpwuid(uid)->pw_name);

        pses[count++] = ps;
    }

    if (filter == 1 && count == 0) {
        printf("invalid user: %s\n", user);
        return 1;
    }

    pses[count] = NULL;

    if (sort == 1)
        qsort(pses, count, sizeof(psinfo*), psinfo_cmp_vsize);
    else if (sort == 2)
        qsort(pses, count, sizeof(psinfo*), psinfo_cmp_cputime);

    print_all_ps(pses, count);

    return 0;
}