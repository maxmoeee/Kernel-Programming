#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Lab task: Use signal handler to reap child processes

// Increase count by 1 after reaping a child
// count is expected to be 1000 after all children finished
int count = 0;
// int child_pid = 0;

void handler(int signal) {
	while (wait(NULL) > 0) {
		count++;
	}
}

int main()
{
	int i;
	for (i = 0; i < 1000; ++i) {
		pid_t pid = fork();
		if (pid == 0) {
			// Child
			sleep(1);
			return 0;
		} else {
			// child_pid = pid;
			signal(SIGCHLD, handler);
		}
	}

	while (1) {
		sleep(1);
		printf("count: %d\n", count);
	}

	return 0;
}