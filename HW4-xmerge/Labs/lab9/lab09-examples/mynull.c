#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

static dev_t dev;
static struct class *cl;
static struct cdev cdev;

static int mynull_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "mynull: open()\n");
	return 0;
}

static int mynull_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "mynull: release()\n");
	return 0;
}

static ssize_t mynull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "mynull: read()\n");
	return 0;
}

static ssize_t mynull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "mynull: write()\n");
	return count;
}

static const struct file_operations mynull_fops = {
	.owner = THIS_MODULE,
	.open = mynull_open,
	.release = mynull_release,
	.read = mynull_read,
	.write = mynull_write
};

static int __init mynull_init(void) 
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

	if (IS_ERR(device = device_create(cl, NULL, dev, NULL, "mynull"))) {
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(device);
	}

	cdev_init(&cdev, &mynull_fops);
	if ((ret = cdev_add(&cdev, dev, 1)) < 0) {
		device_destroy(cl, dev);
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	printk(KERN_INFO "mynull: init()\n");

	return 0;
}

static void __exit mynull_exit(void)
{
	cdev_del(&cdev);
	device_destroy(cl, dev);
	class_destroy(cl);
	unregister_chrdev_region(dev, 1);

	printk(KERN_INFO "mynull: exit()\n");
}

module_init(mynull_init);
module_exit(mynull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author's name");
MODULE_DESCRIPTION("mynull device driver");
