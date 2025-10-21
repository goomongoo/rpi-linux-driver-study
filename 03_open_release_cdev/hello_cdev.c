#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define MY_DEVICE_NAME	"hello_cdev"

static int major;

static int my_open(struct inode *inode, struct file *filp)
{
	pr_info("%s - Major: %d, Minor: %d\n", MY_DEVICE_NAME, imajor(inode), iminor(inode));
	pr_info("%s - filp->f_pos: %lld\n", MY_DEVICE_NAME, filp->f_pos);
	pr_info("%s - filp->f_mode: 0x%x\n", MY_DEVICE_NAME, filp->f_mode);
	pr_info("%s - filp->f_flags: 0x%x\n", MY_DEVICE_NAME, filp->f_flags);

	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	pr_info("%s - File is closed\n", MY_DEVICE_NAME);
	return 0;
}

static struct file_operations fops = {
	.open = my_open,
	.release = my_release
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
