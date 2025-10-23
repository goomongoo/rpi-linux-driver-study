// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include "my_ioctl.h"

#define MY_DEVICE_NAME "hello"
#define MY_CLASS_NAME "hellodev"

static dev_t devt;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static int answer = 42;

static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int not_copied;
	int v;
	struct mystruct s;

	if (_IOC_TYPE(cmd) != MY_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case WR_VAL:
		not_copied = copy_from_user(&v, (int __user *)arg, sizeof(v));
		if (not_copied)
			return -EFAULT;
		answer = v;
		break;
	case RD_VAL:
		v = answer;
		not_copied = copy_to_user((int __user *)arg, &v, sizeof(v));
		if (not_copied)
			return -EFAULT;
		break;
	case GREET:
		not_copied = copy_from_user(&s, (struct mystruct __user *)arg, sizeof(s));
		if (not_copied)
			return -EFAULT;
		for (int i = 0; i < s.repeat; i++)
			pr_info("%s - Hello, %s\n", MY_DEVICE_NAME, s.name);
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = my_ioctl,
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
MODULE_AUTHOR("mongoo");
MODULE_DESCRIPTION("Character device: using ioctl");
