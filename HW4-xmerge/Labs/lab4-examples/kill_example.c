#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void handler(int signal)
{
	switch(signal) {
		case SIGUSR1:
		write(STDOUT_FILENO, "SIGURS1\n", 8);
		break;
	}
}

int main()
{
	pid_t pid = fork();

	if (pid == 0) {
		sleep(5);

		kill(getppid(), SIGUSR1);

		printf("Child finished\n");
	}
	else {
		printf("Waiting for child\n");

		if (signal(SIGUSR1, handler) == SIG_ERR) {
			perror("signal");
		}
		
		wait(NULL);

		printf("Parent finished\n");
	}

	return 0;
}