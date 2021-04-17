#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void handler(int sig)
{
	int status;
	if (waitpid(-1, &status, WUNTRACED) == -1)
		perror("waitpid");

	if (WIFEXITED(status)) {
		printf("Child exited with status %d\n", WEXITSTATUS(status));
	}
	if (WIFSIGNALED(status)) {
		printf("Child terminated by signal %s\n", strsignal(WTERMSIG(status)));
	}
	if (WIFSTOPPED(status)) {
		printf("Child stopped by signal %s\n", strsignal(WSTOPSIG(status)));
	}
}

int main()
{
	pid_t pid;
	int status;

	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
	}

	if ((pid = fork()) == 0) {
		printf("[PID: %d] Child\n", getpid());
		
		execlp("sleep", "sleep", "10", NULL);
	}

	while (1) ;

	return 0;
}
