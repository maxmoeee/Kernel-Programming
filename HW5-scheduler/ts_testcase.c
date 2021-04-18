#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define SCHED_TS 10

void cpu_task(int msecs);
void sleep_task(int msecs, int count);
void mix_task(int cpu_msecs, int sleep_msecs, int count);

void test_case1(void)
{
	struct sched_param sp = {.sched_priority = 0};

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		cpu_task(500);
		exit(EXIT_SUCCESS);
	}
	else {
		if (sched_setscheduler(pid, SCHED_TS, &sp) == -1) {
			perror("sched_setscheduler for child");
		}
		printf("Child-%d created by Parent-%d\n", pid, getpid());
		fflush(stdout);			
	}
}

void test_case2(void)
{
	struct sched_param sp = {.sched_priority = 0};

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		sleep_task(100, 5);
		exit(EXIT_SUCCESS);
	}
	else {
		if (sched_setscheduler(pid, SCHED_TS, &sp) == -1) {
			perror("sched_setscheduler for child");
		}
		printf("Child-%d created by Parent-%d\n", pid, getpid());
		fflush(stdout);			
	}
}

void test_case3(void)
{
	struct sched_param sp = {.sched_priority = 0};

	int i;
	for (i = 0; i < 3; ++i) {
		pid_t pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0) {
			cpu_task(500);
			exit(EXIT_SUCCESS);
		}
		else {
			if (sched_setscheduler(pid, SCHED_TS, &sp) == -1) {
				perror("sched_setscheduler for child");
			}
			printf("Child-%d created by Parent-%d\n", pid, getpid());
			fflush(stdout);			
		}
	}
}

void test_case4(void)
{
	struct sched_param sp = {.sched_priority = 0};

	int i;
	for (i = 0; i < 3; ++i) {
		pid_t pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0) {
			sleep_task(100, 5);
			exit(EXIT_SUCCESS);
		}
		else {
			if (sched_setscheduler(pid, SCHED_TS, &sp) == -1) {
				perror("sched_setscheduler for child");
			}
			printf("Child-%d created by Parent-%d\n", pid, getpid());
			fflush(stdout);			
		}
	}
}

void test_case5(void)
{
	struct sched_param sp = {.sched_priority = 0};

	int i;
	for (i = 0; i < 2; ++i) {
		pid_t pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0) {
			if (i % 2 == 0)
				cpu_task(1000);
			else
				sleep_task(100, 5);
			exit(EXIT_SUCCESS);
		}
		else {
			if (sched_setscheduler(pid, SCHED_TS, &sp) == -1) {
				perror("sched_setscheduler for child");
			}
			printf("Child-%d created by Parent-%d\n", pid, getpid());
			fflush(stdout);			
		}
	}
}

void test_case6(void)
{
	struct sched_param sp = {.sched_priority = 0};

	int i;
	for (i = 0; i < 2; ++i) {
		pid_t pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0) {
			if (i % 2 == 0)
				cpu_task(1000);
			else
				mix_task(100, 100, 5);
			exit(EXIT_SUCCESS);
		}
		else {
			if (sched_setscheduler(pid, SCHED_TS, &sp) == -1) {
				perror("sched_setscheduler for child");
			}
			printf("Child-%d created by Parent-%d\n", pid, getpid());
			fflush(stdout);			
		}
	}
}

void (*test_cases[])(void) = {
	test_case1,
	test_case2,
	test_case3,
	test_case4,
	test_case5,
	test_case6,
};

int main(int argc, const char *argv[])
{
	puts("=== Test program for TS scheduler ===");

	// Parsing command line arguments
	// Syntax: ./myprio_test [test case number]

	if (argc != 2) {
		puts("Error: Wrong input arguments");
		exit(EXIT_FAILURE);
	} 
	int test_case = atoi(argv[1]);

	if (test_case < 1 || test_case > sizeof(test_cases) / sizeof(test_cases[0])) {
		puts("Error: Invalid test case number");
		exit(EXIT_FAILURE);
	}

	printf("Test case: %d\n", test_case);
	printf("Parent-%d started\n", getpid());
	fflush(stdout); // ensure the buffer flush to the stdout


	// We use a higher priority SCHED_FIFO to schedule the main process
	struct sched_param psp = {.sched_priority = 1};

	// Call sched_setscheduler to set this process as SCHED_FIFO
	int ret = sched_setscheduler(0, SCHED_FIFO, &psp);
	if (ret == -1) {
		perror("sched_setscheduler for parent"); // print error message if sched_setscheduler() fails
		exit(EXIT_FAILURE);
	}

	test_cases[test_case - 1]();

	// To avoid zombie/orphan processes, the parent process needs
	// to wait all child processes
	pid_t pid;
	int status;
	while ((pid = wait(&status)) > 0) {
		// Wait for all processes
		printf("Child-%d finished\n", pid);
		fflush(stdout);
	}

	printf("Parent-%d finished\n", getpid());
	return 0;
}

#define LOOP_ITERS_PER_MILLISEC 400000
void burn_1millisecs()
{
	unsigned long long i;
	for (i = 0; i < LOOP_ITERS_PER_MILLISEC; ++i)
		;
}

void cpu_task(int msecs)
{
	int i;
	for (i = 0; i < msecs; ++i)
		burn_1millisecs();
}

static inline void msleep(int msecs)
{
	usleep(msecs * 1000);
}

void sleep_task(int msecs, int count)
{
	int i;
	for (i = 0; i < count; ++i)
		msleep(msecs);
}

void mix_task(int cpu_msecs, int sleep_msecs, int count)
{
	int i;
	for (i = 0; i < count; ++i) {
		cpu_task(cpu_msecs);
		msleep(sleep_msecs);
	}
}

