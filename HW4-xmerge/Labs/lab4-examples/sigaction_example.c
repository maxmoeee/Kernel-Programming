#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define MAXLINE 1024

void handler(int signal)
{
	write(STDOUT_FILENO, "SIGINT\n", 7);
}

int main()
{
	struct sigaction sa;
	char buffer[MAXLINE];

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
	}

	ssize_t count = 0;

	if ((count = read(STDIN_FILENO, buffer, MAXLINE)) != -1) {
		buffer[count] = '\0';
		printf("%s", buffer);
	}
	else {
		if (errno == EINTR) {
			printf("Interrupted\n");
		}
	}

	return 0;
}