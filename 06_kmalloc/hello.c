// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define MY_DEVICE_NAME	"hello"
#define MY_CLASS_NAME	"hellodev"

#define MEMSIZE			64

static dev_t devt;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static int my_open(struct inode *inode, struct file *filp)
{
	filp->private_data = kmalloc(MEMSIZE, GFP_KERNEL);

	if (!filp->private_data)
		return -ENOMEM;

	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	char *pdata = filp->private_data;
	int bytes_to_copy, bytes_copied, bytes_not_copied;

	if (*ppos >= MEMSIZE)
		return 0;

	bytes_to_copy = ((*ppos + count) < MEMSIZE) ? count : (MEMSIZE - *ppos);

	pr_info("%s - read: req=%zu, actual=%d, off=%lld\n", MY_DEVICE_NAME, count, bytes_to_copy, *ppos);

	bytes_not_copied = copy_to_user(buf, &pdata[*ppos], bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	*ppos += bytes_copied;

	if (bytes_not_copied)
		pr_warn("%s - read partial: %d bytes\n", MY_DEVICE_NAME, bytes_copied);

	return bytes_copied;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	char *pdata = filp->private_data;
	int bytes_to_copy, bytes_copied, bytes_not_copied;

	if (*ppos >= MEMSIZE)
		return 0;

	bytes_to_copy = ((*ppos + count) < MEMSIZE) ? count : (MEMSIZE - *ppos);

	pr_info("%s - write: req=%zu, actual=%d, off=%lld\n", MY_DEVICE_NAME, count, bytes_to_copy, *ppos);

	bytes_not_copied = copy_from_user(&pdata[*ppos], buf, bytes_to_copy);
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
	.llseek		= default_llseek,
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
	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;

	ret = cdev_add(&my_cdev, devt, 1);
	if (ret) {
		pr_err("%s - cdev_add failed: %d\n", MY_DEVICE_NAME, ret);
		goto err_unregister_chrdev;
	}

	/* class creation */
	my_class = class_create(MY_CLASS_NAME);
	if (IS_ERR(my_class)) {
		ret = PTR_ERR(my_class);
		pr_err("%s - class_create failed: %d\n", MY_DEVICE_NAME, ret);
		goto err_cdev_del;
	}

	/* /dev nod creation */
	my_device = device_create(my_class, NULL, devt, NULL, "%s%d", MY_DEVICE_NAME, 0);
	if (IS_ERR(my_device)) {
		ret = PTR_ERR(my_device);
		pr_err("%s - device_create failed: %d\n", MY_DEVICE_NAME, ret);
		goto err_class_destroy;
	}

	pr_info("%s - created: major=%d minor=%d (/dev/%s0)\n", MY_DEVICE_NAME, MAJOR(devt), MINOR(devt), MY_DEVICE_NAME);
	return 0;

err_class_destroy:
	class_destroy(my_class);
err_cdev_del:
	cdev_del(&my_cdev);
err_unregister_chrdev:
	unregister_chrdev_region(devt, 1);
	return ret;
}

static void __exit my_exit(void)
{
	device_destroy(my_class, devt);
	class_destroy(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(devt, 1);
	pr_info("%s - unloaded\n", MY_DEVICE_NAME);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pi");
MODULE_DESCRIPTION("Character device: using kmalloc");
