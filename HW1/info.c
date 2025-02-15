#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct cpustat {
    unsigned long total;
    unsigned long used;
} cpustat;

void uptime() {
    FILE* fp = fopen("/proc/uptime", "r");
    int seconds;
    fscanf(fp, "%d %*d", &seconds);
    fclose(fp);
    int day, hour, minute, second;
    day = seconds / (24*3600);
    seconds -= day * (24*3600);
    hour = seconds / 3600;
    seconds -= hour * 3600;
    minute = seconds / 60;
    second = seconds - minute * 60;
    printf("Uptime: %d day(s), %d:%02d:%02d\n", day, hour, minute, second);
}

void meminfo() {
    FILE* fp = fopen("/proc/meminfo", "r");
    char memtotal[25], memfree[128];
    fgets(memtotal, 256, fp);
    fgets(memfree, 256, fp);
    int num_total, num_free;
    sscanf(memtotal, "%*s%d ", &num_total);
    sscanf(memfree, "%*s%d ", &num_free);
    printf("Memory: %d kB / %d kB (%.1f%%)\n", num_total-num_free, num_total, (num_total-num_free)*100.0/num_total);
    fclose(fp);
}

void get_cpu_usage(cpustat* stats, int num, FILE* fp) {
    rewind(fp);
    // skip the first line
    char first_line[256];
    fgets(first_line, 256, fp);

    unsigned long t_user;
    unsigned long t_nice;
    unsigned long t_system;
    unsigned long t_idle;
    unsigned long t_iowait;
    unsigned long t_irq;
    unsigned long t_softirq;
    unsigned long t_steal;
    unsigned long t_guest;
    unsigned long t_guest_nice;

    for (int i = 0; i < num; i++) {
        char line[256];
        fgets(line, 256, fp);
        cpustat st = stats[i];
        sscanf(line, "cpu%*d %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
            &t_user,
            &t_nice,
            &t_system,
            &t_idle,
            &t_iowait,
            &t_irq,
            &t_softirq,
            &t_steal,
            &t_guest,
            &t_guest_nice
        );

        unsigned long t_total = t_user + t_nice + t_system + t_idle + t_iowait + t_irq + t_softirq + t_steal + t_guest + t_guest_nice;
        unsigned long t_used = t_total - t_iowait - t_idle;
        stats[i].total = t_total;
        stats[i].used = t_used;
    }
}

void cpu_usage() {
    FILE* fp = fopen("/proc/stat", "r");
    int num_cpu = 0;

    // skip the first line
    char first_line[256];
    fgets(first_line, 256, fp);

    // count the number of CPUs
    char tmp_line[256];
    while (fgets(tmp_line, 256, fp)) {
        if (strncmp("cpu", tmp_line, 3) == 0)
            num_cpu++;
        else break;
    }

    cpustat stats[num_cpu];
    get_cpu_usage(stats, num_cpu, fp);

    char result[512];
    for (int i = 0; i < num_cpu; i++) {
        if (i != 0)
            strcat(result, "  ");
        char percentage[15];
        sprintf(percentage, "CPU%d: %5.1f%%", i, stats[i].used*100.0/stats[i].total);
        if (i == 0)
            strcpy(result, percentage);
        else
            strcat(result, percentage);
    }
    printf("%s\n", result);
}

int main(int argv, char* args[]) {
    int seconds = 3;
    if (argv == 2) {
        seconds = atoi(args[1]);
        if (seconds <= 0) {
            printf("please input valid time interval!\n");
            return -1;
        }
    }
    while (1) {
        sleep(seconds);
        uptime();
        meminfo();
        cpu_usage();
    }
    return 0;
}