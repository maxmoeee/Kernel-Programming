#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>

static int major = 0;
module_param(major, int, 0444);

static int minor = 0;
module_param(minor, int, 0444);

static int nr_devs = 1;

static struct class *cl;

static int __init hellodev_init(void)
{
	dev_t dev;
	struct device *device;
	int ret;

	if (major > 0) {
		dev = MKDEV(major, minor);
		ret = register_chrdev_region(dev, nr_devs, "comp4511");
	}
	else {
		ret = alloc_chrdev_region(&dev, minor, nr_devs, "comp4511");
		major = MAJOR(dev);
	}

	if (ret) {
		printk(KERN_INFO "devnum: cannot register device number %u:%u\n", major, minor);
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "comp4511"))) {
		unregister_chrdev_region(dev, nr_devs);
		return PTR_ERR(cl);
	}

	if (IS_ERR(device = device_create(cl, NULL, dev, NULL, "hello"))) {
		class_destroy(cl);
		unregister_chrdev_region(dev, nr_devs);
		return PTR_ERR(device);
	}

	return 0;
}

static void __exit hellodev_exit(void)
{
	dev_t dev = MKDEV(major, minor);
	device_destroy(cl, dev);
	class_destroy(cl);
	unregister_chrdev_region(dev, nr_devs);
}

module_init(hellodev_init);
module_exit(hellodev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author's name");
MODULE_DESCRIPTION("hellodev device driver");
