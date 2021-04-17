/**
 * @file myfifo_test.c
 * @brief Create SCHED_MYFIFO jobs
 */
#define _GNU_SOURCE /* For SCHED_XXX */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

/* In user-space, SCHED_MYFIFO is not defined in GNU library */
#define SCHED_MYFIFO 8 

/*
	To test this scheduler, we can't use sleep()
	We need to make the process busy waiting
	Otherwise, the process may be suspended.

	See the implemention below if you are interested
*/
void burn_cpu(int msecs);


int main(int argc, const char *argv[])
{
	puts("=== Test program for MyFIFO ===");

	// Default value: each process runs for 1000ms
	// Otherwise, convert the first parameter as integer
	int msecs = 1000;

	// Default value: 2 child processes
	// Otherwise, convert the second parameter as integer
	int num_process = 2;

	// Parsing command line arguments
	// Syntax: ./myfifo_test [num_process] [running time for each process in ms]

	if ( argc > 3 ) {
		puts("Error: Wrong input arguments");
		return -1;
	} 
	if ( argc > 2 ) 
		msecs = atoi(argv[2]);
	if ( argc > 1 ) 
		num_process = atoi(argv[1]);

	printf("Number of processes: %d\n", num_process);
	printf("Running time for each process: %d(ms)\n", msecs);
	printf("Parent-%d started\n", getpid());
	fflush(stdout); // ensure the buffer flush to the stdout


	// We use a higher priority SCHED_FIFO to schedule the main process
	struct sched_param psp = {.sched_priority = 1};

	// Call sched_setscheduler to set this process as SCHED_FIFO
	int ret = sched_setscheduler(0, SCHED_FIFO, &psp);
	if ( ret == -1 ) {
		perror("sched_setscheduler for parent"); // print error message if sched_setscheduler() fails
		exit(-1);
	}

	// For each child process, we use SCHED_MYFIFO with priority 0
	struct sched_param sp = {.sched_priority = 0};

	int i;
	pid_t pid;
	time_t end;

	// Now, we create "num_process" of child processes
	for (i = 0; i < num_process; i++) {
		pid = fork();

		if ( pid == -1 ) {
			perror("fork"); // print error message if fork() fails
			exit(-1);
		}

		if ( pid == 0 ) { // child process
			burn_cpu(msecs); // busy waiting for msecs
			exit(0); // terminate the child process

		} else { // parent process
			ret = sched_setscheduler( pid, SCHED_MYFIFO, &sp );
			if ( ret == -1 ) {
				perror("sched_setscheduler for child");
			}
			printf("Child-%d created by Parent-%d\n", pid, getpid());
			fflush(stdout);
		}
	}
	// To avoid zombie/orphan processes, the parent process needs
	// to wait all child processes
	int status;
	while ((pid = wait(&status)) > 0) {
	  // Wait for all processes
	  printf("Child-%d finished\n", pid);
	  fflush(stdout);
	}

	printf("Parent-%d finished\n", getpid());
	return 0;

}

/* Implementation of burn_cpu() */
#define LOOP_ITERS_PER_MILLISEC 400000
void burn_1millisecs()
{
	unsigned long long i;
	for (i = 0; i < LOOP_ITERS_PER_MILLISEC; ++i)
		;
}
void burn_cpu(int msecs)
{
	int i;
	for (i = 0; i < msecs; ++i) {
		burn_1millisecs();
	}
}

