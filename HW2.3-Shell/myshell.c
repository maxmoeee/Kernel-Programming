#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_CMDLINE_LEN 1024
#define MAX_ARG_LEN 512
#define MAX_ARGC 128
#define PATH_MAX 128
#define MAX_PIPE 128


typedef struct job {
    struct job* next;
    struct process* list;
    pid_t pgid;
    char* cmd;
} job;

typedef struct process {
    struct process* next;
    pid_t pid;
    int status; // running(0), stopped(1), or terminated(2)
} process;


void show_prompt(FILE* f);
int get_cmd_line(char *cmdline);
void process_cmd(char *cmdline, int async);
void print_jobs();
int exec_internal_cmd(char* cmd);
void cd(char* argv[], int argc);
void exec_job(char* cmdline);
void reap();
void wait_job(job* jb);
void update_process_status(pid_t pid, int status);
job* get_job(pid_t pgid);
void mark_job_running(job *jb);
void resume(char* argv[], int argc, int type);
void sigint_handler(int signal);
int process_redirection(const char* cmdline, int i);

// helper functions
int is_delim(char c, char delim);
int is_stopped(job* jb);
int is_terminated(int status);
void change_status(process* ps, int status);
void copy_string(char const *dest, char const *source, size_t max_len);

job* head = NULL;
job* current = NULL;
extern char **environ;
extern int errno;
int exit_status;

int main() {
	char cmdline[MAX_CMDLINE_LEN];
    signal(SIGINT, sigint_handler);
    signal(SIGTTOU, SIG_IGN);

    job* head;

	while (1)
	{
		show_prompt(stdout);

		if (get_cmd_line(cmdline) == -1 )
			continue; /* empty line handling */

        exec_job(cmdline);
	}

	return 0;
}

int get_path(const char* cmdline, char* path, int i) {
    while (is_delim(cmdline[++i], ' '));
    int j = i;
    while (i < strlen(cmdline) && !is_delim(cmdline[++i], ' '));
    strncpy(path, cmdline+j, i-j);
    path[i-j] = '\0';
    return i;
}

/* Helper function to process redirection, increamen*/
int process_redirection(const char* cmdline, int i) {
    char c = cmdline[i];
    int mode = O_TRUNC;
    char path[PATH_MAX];

    if (c == '>') {
        if (cmdline[i+1] == '>') {
            mode = O_APPEND;
            i++;
        }
        i = get_path(cmdline, path, i);
        int out = open(path, O_RDWR | O_CREAT | mode, 0666);
        dup2(out, 1);
        close(out);
        while (is_delim(cmdline[++i], ' '));
    } else if (c == '&' && cmdline[i+1] == '>') {
        i = get_path(cmdline, path, ++i);
        int out = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        dup2(out, 1);
        dup2(out, 2);
        close(out);
        while (is_delim(cmdline[++i], ' '));
    } else if (c == '<') {
        i = get_path(cmdline, path, i);
        int in = open(path, O_RDONLY, 0666);
        if (in < 0)
            perror(path);
        else {
            dup2(in, 0);
            close(in);
        }
        while (is_delim(cmdline[++i], ' '));
    }

    return i;
}

void process_cmd(char *cmdline, int async) {
    char* argv[MAX_ARGC];
    int argc = 0; // number of arguments

    int i = 0; //pointer for the cmdline
    int j = 0; //pointer for the current argument
    char* arg = malloc(MAX_ARG_LEN); //current argument
    char delim = ' ';

    // skip leading white spaces
    while (cmdline[i] == ' ' || cmdline[i] == '\t') ++i;

    while (i < strlen(cmdline)) {
        char c = cmdline[i];

        if (is_delim(c, delim)) {
            if (delim == '\'' || delim == '\"')
                delim = ' ';

            arg[j] = '\0';
            argv[argc++] = arg;

            arg = malloc(MAX_ARG_LEN);
            j = 0;

            c = cmdline[++i];
            while (c == ' ' || c == '\t') { // when there are more than one spaces
                c = cmdline[++i];
            }
            continue;
        } else if (i == strlen(cmdline)-1) {
            if (delim == ' ')
                arg[j++] = c;
            arg[j] = '\0';
            argv[argc++] = arg;
            break;
        }

        // process quote
        if ((c == '\"' || c == '\'') && delim == ' ') {
            delim = c;
            i++;
            continue;
        }

        // process $special variables
        if (delim != '\'' && cmdline[i] == '$') {
            if (cmdline[i+1] == '$' || cmdline[i+1] == '?') {
                if (cmdline[i+1] == '$')
                    j += sprintf(arg+j, "%d", getpid());
                else
                    j += sprintf(arg+j, "%d", exit_status);

                i += 2;
                if (i >= strlen(cmdline)) {
                    arg[j] = '\0';
                    argv[argc++] = arg;
                }
                continue;
            }
        }

        int tmp = process_redirection(cmdline, i);
        if (tmp == i) {
            arg[j++] = c;
            i++;
        } else
            i = tmp;
    }

    argv[argc] = (char*)NULL;
    execvpe(argv[0], argv, environ);

    perror("command not found");
    exit(-1);
}

int exec_internal_cmd(char* cmdline) {
    char cmd[strlen(cmdline)];
    strcpy(cmd, cmdline);
    reap();

    if (!strcmp(cmd, "exit") || !strcmp(cmd, "exit &"))
        exit(0);

    if (!strcmp(cmd, "jobs")) {
        print_jobs();
        return 1;
    }

    char* argv[MAX_ARG_LEN];
    int argc = 0;
    char* token = strtok(cmd, " ");
    while (token != NULL) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = token;

    if (!strcmp(argv[0], "cd")) {
        cd(argv, argc);
        return 1;
    }

    if (!strcmp(argv[0], "bg")) {
        resume(argv, argc, 0);
        return 1;
    }

    if (!strcmp(argv[0], "fg")) {
        resume(argv, argc, 1);
        return 1;
    }

    return 0;
}

void exec_job(char* cmdline) {
    // fprintf(stderr, cmdline);
    // cmd reserved for job's info
    char* cmd = malloc(strlen(cmdline)+1);
    strcpy(cmd, cmdline);

    // check async &
    int async = 0;
    int i = strlen(cmdline) - 1;
    char c = cmdline[i];
    while ((c == ' ' || c == '\t') && i-- > 0) c = cmdline[i];
    if (c == '&') {
        async = 1;
        cmdline[i] = '\0';
    }

    // process pipe
    char *cmdlines[MAX_PIPE];
    int count = 0;
    char *tmp = cmdline;
    char* p = strchr(cmdline, '|');
    while (p != NULL) {
        *p = '\0';
        cmdlines[count++] = tmp;
        tmp = p+1;
        while (*tmp == ' ' || *tmp == '\t') tmp++;
        p = strchr(tmp, '|');
    }
    cmdlines[count++] = tmp;

    // assume no internal cmd is part of a pipeline
    if (count == 1 && exec_internal_cmd(cmdlines[0]))
        return;

    // connect pipes
    pid_t pgid = 0;
    job* jb;
    int in = 0, fd[2];
    reap();

    for (i = 0; i < count; i++) {
        pipe(fd);
        pid_t pid;
        if ((pid = fork()) == 0) {
            if (i != 0) { // except for the 1st cmd
                close(0);
                dup(in);
                close(fd[0]);
                close(in);
            }
            if (i != count - 1) { // except for the last cmd
                close(1);
                dup(fd[1]);
                close(fd[1]);
            }
            process_cmd(cmdlines[i], async);
        } else {
            if (pgid == 0) {
                pgid = pid;
                process* ps = (process*) malloc(sizeof(process));
                ps->pid = pid;
                ps->next = NULL;
                ps->status = 0;
                jb = (job*) malloc(sizeof(job));
                jb->cmd = cmd;
                jb->pgid = pgid;
                jb->list = ps;
                jb->next = head;
                head = jb;
            } else {
                process* ps = (process*) malloc(sizeof(process));
                ps->pid = pid;
                ps->status = 0;
                ps->next = head->list;
                head->list = ps;
            }
            setpgid(pid, pgid);
            close(fd[1]);
            in = fd[0];
        }
    }

    if (!async) wait_job(jb);
}

/* Check for processes with status changes. Non-blocking. */
void reap() {
    // reap once before exec
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // fprintf(stderr, "%s\n", "!!!");
        update_process_status(pid, status);
    }
}

/* wait for job in the foreground */
void wait_job(job* jb) {
    pid_t pgid = jb->pgid;
    tcsetpgrp(STDIN_FILENO, pgid);
    int status;
    int terminated = 1, stopped = 1;

    process* ps = jb->list;
    while (ps) {
        waitpid(ps->pid, &status, WUNTRACED);
        change_status(ps, status);
        terminated &= (ps->status == 2);
        stopped &= (ps->status == 1);
        ps = ps->next;
    }

    if (terminated) {
        head = head->next;
    } else if (stopped) {
        current = jb;
    }

    tcsetpgrp(STDIN_FILENO, getpid());
}

/* Update the status no. of a process. */
void change_status(process* ps, int status) {
    if (WIFEXITED(status) || WIFSIGNALED(status))
        ps->status = 2; // terminated
    else if (WIFSTOPPED(status))
        ps->status = 1; // stopped
    else
        ps->status = 0; // running
}

/* Update the process's status */
void update_process_status(pid_t pid, int status) {
    int terminated = 1, stopped = 1, found = 0;
    job* cur = head, *prev = NULL;
    while (cur != NULL) {
        process* ps = cur->list;
        while (ps != NULL) {
            if (ps->pid == pid) {
                change_status(ps, status);
                found = 1;
            }
            terminated &= (ps->status == 2);
            stopped &= (ps->status == 1);
            ps = ps->next;
        }
        if (found) break;
        prev = cur;
        cur = cur->next;
        terminated = 1;
        stopped = 1;
    }

    if (cur == NULL)
        fprintf(stderr, "%s\n", "Your code is incorrect!");

     // remove a job if all its processes have terminated
    if (terminated) {
        if (current == cur)
            current = cur->next;

        if (prev != NULL) {
            prev->next = cur->next;
        } else {
            head = head->next;
        }
    } else if (stopped) {
        current = cur;
    }
}

void show_prompt(FILE* f) {
    struct passwd *pw = getpwuid(getuid());
    char* home_dir = pw->pw_dir;
    int len = strlen(home_dir);

    char cur_dir[PATH_MAX];
    getcwd(cur_dir, PATH_MAX);

    char dir[PATH_MAX];

    if (strcmp(home_dir, cur_dir) == 0) {
        sprintf(dir, "~");
    } else if (strncmp(home_dir, cur_dir, len) == 0) {
        strcpy(dir, cur_dir+len+1);
    } else {
        strcpy(dir, cur_dir);
    }

	fprintf(f, "[%s] myshell> ", dir);
}

int get_cmd_line(char *cmdline) {
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LEN, stdin)) // note it is getting input from the stdin, so myshell> won't be included
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';

    i = 0;
    while (i < n && (cmdline[i] == ' ' || cmdline[i] == '\t')) {
        ++i;
    }
    if (i == n) { // empty command
        return -1;
    }

    // remove trailing spaces
    while (cmdline[--n] == ' ' || cmdline[--n] == '\t') {
        cmdline[n] = '\0';
    }

    return 0;
}

void cd(char* argv[], int argc) {
    if (argc == 1) {
        char* home_dir = getpwuid(getuid())->pw_dir;
        chdir(home_dir);
    } else if (argc > 2) {
        printf("cd: string not in pwd: %s\n", argv[1]);
    } else if (argc == 2) {
        if (chdir(argv[1])) {
            switch (errno)
            {
            case EACCES:
                perror("Permission not allowed");
                break;
            case EFAULT:
                perror("Path points outside your accessible address space");
                break;
            case EIO:
                perror("An I/O error occurred");
                break;
            case ELOOP:
                perror("Too many symbolic links were encountered in resolving path");
                break;
            case ENAMETOOLONG:
                perror("path is too long");
                break;
            case ENOENT:
                perror("The directory does not exist");
                break;
            case ENOMEM:
                perror("Insufficient kernel memory was available");
                break;
            case ENOTDIR:
                perror("Target is not a directory");
                break;
            }
        }
    }
}

/* Get job from pgid */
job* get_job(pid_t pgid) {
    job* jb = head;
    while (jb) {
        if (jb->pgid == pgid)
            return jb;
        jb = jb->next;
    }
    if (!jb) fprintf(stderr, "%s\n", "Job is not resumed correctly!");
    return jb;
}

/* Set the status of the processes in the job as running */
void mark_job_running(job* jb) {
    process* ps = jb->list;
    while (ps) {
        ps->status = 1;
        ps = ps->next;
    }
}

void resume(char* argv[], int argc, int type) {
    pid_t pgid;
    if (argc == 1) {
        if (current)
            pgid = current->pgid;
        else if (head)
            pgid = head->pgid;
        else {
            fprintf(stderr, "%s\n", "Job is already running!");
            return;
        }
    } else if (argc == 2) {
        pgid = atoi(argv[1]);
    }

    job* jb = get_job(pgid);
    if (type == 0) { // bg
        if (kill(-pgid, SIGCONT) < 0)
            perror("Kill -SIGCONT");
        else
            mark_job_running(jb);
    } else { // fg
        if (kill(-pgid, SIGCONT) < 0)
            perror("Kill -SIGCONT");
        else {
            mark_job_running(jb);
            wait_job(jb);
        }
    }
}

void print_jobs() {
    job* cur = head;
    int i = 1;
    while (cur) {
        printf("[%d]\t%d\t%s\n", i, cur->pgid, cur->cmd);
        i++;
        cur = cur->next;
    }
}

/* Helper function: check whether a job is stopped; else it's a running background job. */
int is_stopped(job* jb) {
    int result = 1;
    process* ps = jb->list;
    while (ps != NULL) {
        result &= WIFSTOPPED(ps->status);
        ps = ps->next;
    }
    return result;
}

int is_delim(char c, char delim) {
    if (delim == c)
        return 1;
    if (delim == ' ' && c == '\t')
        return 1;
    return 0;
}

void sigint_handler(int signal) {
    fprintf(stderr, "\nPlease use the exit command.\n");
    show_prompt(stderr);
}
