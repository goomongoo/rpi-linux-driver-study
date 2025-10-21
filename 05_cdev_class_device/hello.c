// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define MY_DEVICE_NAME	"hello"
#define MY_CLASS_NAME	"hellodev"

static dev_t devt;
static struct cdev hello_cdev;
static struct class *hello_class;
static struct device *hello_device;

static char my_buf[256];

static int my_open(struct inode *inode, struct file *filp)
{
	pr_info("%s - open()\n", MY_DEVICE_NAME);
	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	pr_info("%s - release()\n", MY_DEVICE_NAME);
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int bytes_to_copy, bytes_copied, bytes_not_copied;

	if (*ppos >= sizeof(my_buf))
		return 0;

	bytes_to_copy = ((*ppos + count) < sizeof(my_buf)) ? count : (sizeof(my_buf) - *ppos);

	pr_info("%s - read: req=%zu, actual=%d, off=%lld\n", MY_DEVICE_NAME, count, bytes_to_copy, *ppos);

	bytes_not_copied = copy_to_user(buf, &my_buf[*ppos], bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	*ppos += bytes_copied;

	if (bytes_not_copied)
		pr_warn("%s - read partial: %d bytes\n", MY_DEVICE_NAME, bytes_copied);

	return bytes_copied;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int bytes_to_copy, bytes_copied, bytes_not_copied;

	if (*ppos >= sizeof(my_buf))
		return 0;

	bytes_to_copy = ((*ppos + count) < sizeof(my_buf)) ? count : (sizeof(my_buf) - *ppos);

	pr_info("%s - write: req=%zu, actual=%d, off=%lld\n", MY_DEVICE_NAME, count, bytes_to_copy, *ppos);

	bytes_not_copied = copy_from_user(&my_buf[*ppos], buf, bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	*ppos += bytes_copied;

	if (bytes_not_copied)
		pr_warn("%s - write partial: %d bytes\n", MY_DEVICE_NAME, bytes_copied);

	return bytes_copied;
}

static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.open		= my_open,
	.release	= my_release,
	.read		= my_read,
	.write		= my_write,
};

static int __init my_init(void)
{
	int ret;

	/* major/minor allocation */
	ret = alloc_chrdev_region(&devt, 0, 1, MY_DEVICE_NAME);
	if (ret) {
		pr_err("%s - alloc_chrdev_region failed: %d\n", MY_DEVICE_NAME, ret);
		return ret;
	}

	/* cdev init/register */
	cdev_init(&hello_cdev, &fops);
	hello_cdev.owner = THIS_MODULE;

	ret = cdev_add(&hello_cdev, devt, 1);
	if (ret) {
		pr_err("%s - cdev_add failed: %d\n", MY_DEVICE_NAME, ret);
		goto err_unregister_chrdev;
	}

	/* class creation */
	hello_class = class_create(MY_CLASS_NAME);
	if (IS_ERR(hello_class)) {
		ret = PTR_ERR(hello_class);
		pr_err("%s - class_create failed: %d\n", MY_DEVICE_NAME, ret);
		goto err_cdev_del;
	}

	/* /dev nod creation */
	hello_device = device_create(hello_class, NULL, devt, NULL, "%s%d", MY_DEVICE_NAME, 0);
	if (IS_ERR(hello_device)) {
		ret = PTR_ERR(hello_device);
		pr_err("%s - device_create failed: %d\n", MY_DEVICE_NAME, ret);
		goto err_class_destroy;
	}

	pr_info("%s - created: major=%d minor=%d (/dev/%s0)\n", MY_DEVICE_NAME, MAJOR(devt), MINOR(devt), MY_DEVICE_NAME);
	return 0;

err_class_destroy:
	class_destroy(hello_class);
err_cdev_del:
	cdev_del(&hello_cdev);
err_unregister_chrdev:
	unregister_chrdev_region(devt, 1);
	return ret;
}

static void __exit my_exit(void)
{
	device_destroy(hello_class, devt);
	class_destroy(hello_class);
	cdev_del(&hello_cdev);
	unregister_chrdev_region(devt, 1);
	pr_info("%s - unloaded\n", MY_DEVICE_NAME);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pi");
MODULE_DESCRIPTION("Character device: cdev + class + device (hello/hellodev)");
