#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	while (1) {
		fprintf(stderr, "[PID: %d PPID: %d PGID: %d SID: %d] %s", getpid(), getppid(), getpgrp(), getsid(0), argv[0]);

		int i;
		for (i = 1; i < argc; ++i)
			fprintf(stderr, " %s", argv[i]);
		fprintf(stderr, "\n");

		sleep(10);
	}
}