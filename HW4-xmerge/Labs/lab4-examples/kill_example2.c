#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int running = 1;

void handler(int signal)
{
	switch(signal) {
		case SIGINT:
		write(STDOUT_FILENO, "SIGINT\n", 7);
		break;
		case SIGTERM:
		write(STDOUT_FILENO, "SIGTERM\n", 8);
		running = 0;
	}
}

int main()
{
	if (signal(SIGINT, handler) == SIG_ERR || signal(SIGTERM, handler) == SIG_ERR) {
		perror("signal");
	}

	printf("PID: %d\n", getpid());

	while (running) {
		sleep(1);
	}
}