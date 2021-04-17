#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

void handler(int signal)
{
	// UNSAFE!
	printf("[PID: %d PPID: %d PGID: %d] Received SIGUSR1\n", getpid(), getppid(), getpgrp());
}

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
			
			struct sigaction sa;
			sa.sa_handler = handler;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = 0;

			if (sigaction(SIGUSR1, &sa, NULL) == -1) {
				perror("sigaction");
			}

			printf("[PID: %d PPID: %d PGID: %d] Child\n", getpid(), getppid(), getpgrp());
			sleep(2);
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
	killpg(pgid, SIGUSR1); // kill(-pgid, SIGUSR1) also works
	printf("[PID: %d PPID: %d PGID: %d] Parent\n", getpid(), getppid(), getpgrp());
	return 0;
}
