#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

static int major = 0;
module_param(major, int, 0444);

static int minor = 0;
module_param(minor, int, 0444);

static int nr_devs = 1;
module_param(nr_devs, int, 0444);

static int __init devnum_init(void)
{
	dev_t dev;
	int ret, i;

	if (major > 0) {
		dev = MKDEV(major, minor);
		ret = register_chrdev_region(dev, nr_devs, "comp4511");
	}
	else {
		ret = alloc_chrdev_region(&dev, minor, nr_devs, "comp4511");
		major = MAJOR(dev);
	}

	if (ret == 0)
		for (i = 0; i < nr_devs; ++i)
			printk(KERN_INFO "devnum: registered device number %u:%u\n", major, minor + i);
	else
		printk(KERN_INFO "devnum: cannot register device number %u:%u\n", major, minor);

	return ret;
}

static void __exit devnum_exit(void)
{
	dev_t dev = MKDEV(major, minor);
	unregister_chrdev_region(dev, nr_devs);
}

module_init(devnum_init);
module_exit(devnum_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author's name");
MODULE_DESCRIPTION("devnum device driver");
