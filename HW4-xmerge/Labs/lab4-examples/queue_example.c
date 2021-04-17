#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int count = 0;

void handler(int signal)
{
	switch(signal) {
		case SIGUSR1:
		++count;
		break;
	}
}

int main()
{
	if (signal(SIGUSR1, handler) == SIG_ERR) {
		perror("signal");
	}

	pid_t pid = fork();

	if (pid == 0) {
		int i;
		for (i = 0; i < 100000; ++i)
			kill(getppid(), SIGUSR1);

		printf("Child finished\n");
	}
	else {
		printf("Waiting for child\n");
		
		wait(NULL);

		printf("Parent finished. Count = %d\n", count);
	}

	return 0;
}