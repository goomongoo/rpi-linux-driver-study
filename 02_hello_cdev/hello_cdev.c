#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define MY_DEVICE_NAME	"hello_cdev"

static int major;

static ssize_t my_read (struct file *f, char __user *u, size_t l, loff_t *o)
{
	pr_info("%s - Read is called!\n", MY_DEVICE_NAME);
	return 0;
}

static struct file_operations fops = {
	.read = my_read
};

static int __init my_init(void)
{
	major = register_chrdev(0, MY_DEVICE_NAME, &fops);
	
	if (major < 0)
	{
		pr_err("%s - Error registering chrdev\n", MY_DEVICE_NAME);
		return major;
	}

	pr_info("%s - Major Device Number: %d\n", MY_DEVICE_NAME, major);
	return 0;
}

static void __exit my_exit(void)
{
	unregister_chrdev(major, MY_DEVICE_NAME);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("pi");
MODULE_DESCRIPTION("A sample driver for registering a character device");
