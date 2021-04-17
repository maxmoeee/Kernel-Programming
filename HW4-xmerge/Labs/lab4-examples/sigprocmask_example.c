#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void handler(int signal)
{
	write(STDOUT_FILENO, "SIGINT\n", 7);
}

int main()
{
	sigset_t block, prev;

	sigemptyset(&block);
	sigaddset(&block, SIGINT);

	signal(SIGINT, handler);

	printf("Before sigprocmask()\n");

	if (sigprocmask(SIG_BLOCK, &block, &prev) == -1) {
		perror("sigprocmask");
	}

	sleep(5);
	
	if (sigprocmask(SIG_SETMASK, &prev, NULL) == -1) {
		perror("sigprocmask");
	}

	printf("After sigprocmask()\n");

	sleep(5);

	return 0;
}