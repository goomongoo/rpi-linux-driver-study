#define DEBUG
#include <linux/module.h>
#include <linux/init.h>

static int __init my_init(void)
{
	pr_debug("%s - Hello, Kernel!\n", __func__);
	return 0;
}

static void __exit my_exit(void)
{
	pr_debug("%s - Goodbye, Kernel!\n", __func__);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ssafy");
MODULE_DESCRIPTION("A simple Hello World LKM");
