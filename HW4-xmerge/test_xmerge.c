#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define __NR_xmerge 336
#define BUFSIZE 1024
#define MAX_FILENAME 128

extern int errno;

typedef struct xmerge_params {
	char *outfile;
	char **infiles;
	unsigned int num_files;
	int oflags;
	mode_t mode;
	int *ofile_count;
} xmerge_params;

long usr_xmerge(void *params, size_t argslen);

int main(int argc, char *argv[])
{
	xmerge_params* params = malloc(sizeof(xmerge_params));
	size_t argslen = sizeof(xmerge_params);
	params->outfile = (char *) malloc(MAX_FILENAME);
	params->oflags = O_WRONLY | O_CREAT | O_APPEND;
	params->mode = 0600; // S_IRUSR | S_IWUSR
	params->ofile_count = malloc(sizeof(int));

	// parse args to fill in params
	int opt;
	while ((opt = getopt(argc, argv, "actem:")) != -1) {
		// printf("%c\n", opt);
		switch(opt) {
			case 'a':
				params->oflags |= O_APPEND;
				break;
			case 'c':
				params->oflags |= O_CREAT;
				break;
			case 't':
				params->oflags |= O_TRUNC;
				break;
			case 'e':
				params->oflags |= O_EXCL;
				break;
			case 'm':
				params->mode = strtol(optarg, NULL, 8);
				break;
		}
	}

	strcpy(params->outfile, argv[optind++]);
	params->num_files = argc - optind;
	params->infiles = (char **) malloc(sizeof(char*) * params->num_files);
	int i = 0;
	while (optind < argc) {
		(params->infiles)[i] = (char*) malloc(MAX_FILENAME);
		strcpy((params->infiles)[i++], argv[optind++]);
	}

	long bytes_num = syscall(__NR_xmerge, params, argslen);
	if (bytes_num > 0)
		printf("%ld bytes merged\n%d file(s) merged\n", bytes_num, *(params->ofile_count));
	else
		printf("ERRNO: %d\n", -errno);

	free(params->outfile);
	for (i = 0; i < params->num_files; i++)
		free((params->infiles)[i]);
	free(params->ofile_count);
	free(params);

	return 0;
}

/*
	User space function for xmerge:
	Return the number of bytes concatenated.
	On error, return -errno.
*/
long usr_xmerge(void *params, size_t argslen) {
	int i;
	char buffer[BUFSIZE];
	int num_bytes = 0;
	int ofile_count = 0;

	xmerge_params* p = params;
	// kernel: do copy_from_user()

	int in; // the current input file
	// printf("%d\n", p->mode);
	int out = open(p->outfile, p->oflags, p->mode);
	if (out < 0)
		return -errno;

	for (i = 0; i < p->num_files; i++) {
		in = open((p->infiles)[i], O_RDONLY);
		if (in < 0)
			goto quit;

		while (1) {
			int bytes_read = read(in, buffer, BUFSIZE);
			if (bytes_read < 0)
				goto quit;
			if (bytes_read == 0)
				break;

			int bytes_write = write(out, buffer, bytes_read);
			if (bytes_write < 0)
				goto quit;
			num_bytes += bytes_write;
		}
		close(in);
		ofile_count++;
	}

	close(out);
	// kernel: copy_to_user(p->ofile_count, &ofile_count, sizeof(int));
	*(p->ofile_count) = ofile_count;
	return num_bytes;

	quit:
		close(in);
		close(out);
		*(p->ofile_count) = ofile_count;
		return -errno;
}
