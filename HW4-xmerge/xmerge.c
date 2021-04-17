/* comp4511/xmerge.c */
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/kernel.h>

typedef struct xmerge_params {
	char __user *outfile;
	char __user **infiles;
	unsigned int num_files;
	int oflags;
	mode_t mode;
	int __user *ofile_count;
} xmerge_params;

/*
	Return the number of bytes concatenated.
	On error, return -errno.
*/
SYSCALL_DEFINE2(xmerge, void __user *, params, size_t, argslen) // Fill in the arguments
{
	int i;
	int num_bytes = 0;
	int ofile_count = 0;
	int errno;
	int in, out; // the current input file/ output file
	char buffer[PAGE_SIZE];
	mm_segment_t old_fs;

	xmerge_params* p = params;
	char **infiles = (char **) kmalloc(sizeof(char*) * p->num_files, GFP_KERNEL);
	if (copy_from_user(infiles, p->infiles, sizeof(char*) * p->num_files)) {
		errno = -EFAULT;
		goto quit;
	}

	out = ksys_open(p->outfile, p->oflags, p->mode);
	if (out < 0) {
		errno = out;
		// printk(KERN_INFO "ksysopen outfile: %d\n", errno);
		goto quit;
	}

	old_fs = get_fs();
	set_fs(get_ds());

	for (i = 0; i < p->num_files; i++) {
		in = ksys_open(infiles[i], O_RDONLY, 0444);
		if (in < 0) {
			errno = in;
			// printk(KERN_INFO "ksysopen infiles: %d\n", errno);
			goto close1_and_quit;
		}

		while (1) {
			int bytes_read, bytes_write;

			bytes_read = ksys_read(in, buffer, PAGE_SIZE);
			if (bytes_read < 0) {
				errno = bytes_read;
				goto close2_and_quit;
			}
			if (bytes_read == 0)
				break;

			bytes_write = ksys_write(out, buffer, bytes_read);
			if (bytes_write < 0) {
				errno = bytes_write;
				goto close2_and_quit;
			}
			num_bytes += bytes_write;
		}
		ksys_close(in);
		ofile_count++;
	}

	set_fs(old_fs);
	ksys_close(out);
	copy_to_user(p->ofile_count, &ofile_count, sizeof(int));
	kfree(infiles);
	return num_bytes;

	// exit points
	close2_and_quit:
		ksys_close(in);
	close1_and_quit:
		ksys_close(out);
		copy_to_user(p->ofile_count, &ofile_count, sizeof(int));
		set_fs(old_fs);
	quit:
		kfree(infiles);
		// printk(KERN_INFO "before return: %d\n", errno);
		return errno;
}
