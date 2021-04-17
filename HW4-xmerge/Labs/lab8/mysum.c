#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int from;
module_param(from, int, 0644);

static int to;
module_param(to, int, 0644);

static int result;
module_param(result, int, 0644);

void sum(int from, int to)
{
	result = 0;
	if (from > to)
		return;

	while (from <= to) {
		result += from;
		from++;
	}
}
EXPORT_SYMBOL(sum);

static int __init sum_init(void)
{
	printk(KERN_INFO "Enter mysum\n");
	sum(from, to);
	return 0;
}

static void __exit sum_exit(void)
{
	printk(KERN_INFO "Exit mysum\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CHEN, Xumeng");
MODULE_DESCRIPTION("Module suming integers");

module_init(sum_init);
module_exit(sum_exit);
