#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>

#include <linux/slab.h>

static dev_t dev;
static struct class *cl;

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	struct cdev cdev;
};
static struct scull_dev scull_device;

#define SCULL_QUANTUM 4000
#define SCULL_QSET    1000

static int scull_quantum = SCULL_QUANTUM;
module_param(scull_quantum, int, 0644);

static int scull_qset = SCULL_QSET;
module_param(scull_qset, int, 0644);

static int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; ++i)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}

	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;

	return 0;
}

static int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;
	printk(KERN_INFO "scull: open()\n");

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
	}

	return 0;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "scull: release()\n");
	return 0;
}

static struct scull_qset *scull_follow(struct scull_dev *dev, int n)
{
	struct scull_qset *qs = dev->data;

	if (!qs) {
		qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;
		memset(qs, 0, sizeof(struct scull_qset));
	}

	for (; n--; qs = qs->next) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;
			memset(qs->next, 0, sizeof(struct scull_qset));
		}
	}

	return qs;
}

static ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;

	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;

	if (*f_pos >= dev->size)
		return 0;

	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	item = (long) *f_pos / itemsize;
	rest = (long) *f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	printk(KERN_INFO "scull: read() with count=%zd, s_pos=%d, q_pos=%d\n", count, s_pos, q_pos);

	dptr = scull_follow(dev, item);

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		return 0;

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count))
		return -EFAULT;

	*f_pos += count;
	return count;
}

static ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;

	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;

	item = (long) *f_pos / itemsize;
	rest = (long) *f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	printk(KERN_INFO "scull: write() with count=%zd, s_pos=%d, q_pos=%d\n", count, s_pos, q_pos);

	dptr = scull_follow(dev, item);
	if (dptr == NULL)
		return -ENOMEM;

	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			return -ENOMEM;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			return -ENOMEM;
	}

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count))
		return -EFAULT;

	*f_pos += count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;

	return count;
}

static const struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.read = scull_read,
	.write = scull_write
};

static int __init scull_init(void) 
{
	int ret;
	struct device *device;

	if ((ret = alloc_chrdev_region(&dev, 0, 1, "comp4511")) < 0) {
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "comp4511"))) {
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(cl);
	}

	if (IS_ERR(device = device_create(cl, NULL, dev, NULL, "scull"))) {
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(device);
	}

	cdev_init(&scull_device.cdev, &scull_fops);
	if ((ret = cdev_add(&scull_device.cdev, dev, 1)) < 0) {
		device_destroy(cl, dev);
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	return 0;
}

static void __exit scull_exit(void)
{
	scull_trim(&scull_device);
	cdev_del(&scull_device.cdev);
	device_destroy(cl, dev);
	class_destroy(cl);
	unregister_chrdev_region(dev, 1);
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_DESCRIPTION("scull device driver");
