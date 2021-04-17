#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void handler(int signal)
{
	write(STDOUT_FILENO, "SIGINT\n", 7);
}

int main()
{
	if (signal(SIGINT, handler) == SIG_ERR) {
		perror("signal");
	}

	while (1) {
		sleep(1);
	}
}