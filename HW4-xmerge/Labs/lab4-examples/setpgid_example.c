#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
	pid_t pid, pgid;
	int i;
	for (i = 0; i < 3; ++i) {
		if ((pid = fork()) == 0) {
			if (i == 0)
				pgid = 0;
			if (setpgid(0, pgid))
				perror("child setpgid");
			printf("[PID: %d PPID: %d PGID: %d] Child\n", getpid(), getppid(), getpgrp());
			sleep(1);
			return 0;
		}
		else {
			if (i == 0)
				pgid = pid;
			if (setpgid(pid, pgid))
				perror("parent setpgid");
		}
	}
	sleep(1);
	printf("[PID: %d PPID: %d PGID: %d] Parent\n", getpid(), getppid(), getpgrp());
	return 0;
}
