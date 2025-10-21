// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define MY_DEVICE_NAME	"hello_cdev"

static int major;
static char my_buf[256];

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int bytes_to_copy;
	int bytes_copied;
	int bytes_not_copied;

	bytes_to_copy = (count + *ppos) < sizeof(my_buf) ? count : (sizeof(my_buf) - *ppos);

	pr_info("%s - Read is called. Requested to read %ld bytes. Actually reading %d bytes. The offset is %lld\n", MY_DEVICE_NAME, count, bytes_to_copy, *ppos);

	if (*ppos >= sizeof(my_buf))
		return 0;

	bytes_not_copied = copy_to_user(buf, &my_buf[*ppos], bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	*ppos += bytes_copied;

	if (bytes_not_copied)
		pr_warn("%s - Could only read %d bytes\n", MY_DEVICE_NAME, bytes_copied);

	return bytes_copied;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int bytes_to_copy;
	int bytes_copied;
	int bytes_not_copied;

	bytes_to_copy = (count + *ppos) < sizeof(my_buf) ? count : (sizeof(my_buf) - *ppos);

	pr_info("%s - Write is called. Requested to write %ld bytes. Actually writing %d bytes. The offset is %lld\n", MY_DEVICE_NAME, count, bytes_to_copy, *ppos);

	if (*ppos >= sizeof(my_buf))
		return 0;

	bytes_not_copied = copy_from_user(&my_buf[*ppos], buf, bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	*ppos += bytes_copied;

	if (bytes_not_copied)
		pr_warn("%s - Could only write %d bytes\n", MY_DEVICE_NAME, bytes_copied);

	return bytes_copied;
}

static const struct file_operations fops = {
	.read = my_read,
	.write = my_write
};

static int __init my_init(void)
{
	major = register_chrdev(0, MY_DEVICE_NAME, &fops);

	if (major < 0) {
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
