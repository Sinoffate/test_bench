/**
 * @author Jered Wiegel
 * @date 18 Dec 2021
 * @version 0.1
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/errno.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jered Wiegel");
MODULE_INFO(version, "0.1");

#define MAX_DEV 1
#define BUF_LEN 80
#define IOCTL_MEME_BASE 0x69
#define IOCTL_MEME_INCREMENT         _IOWR(IOCTL_MEME_BASE, 0x0, struct meme_increment_t)

struct meme_increment_t {
    uint64_t target;
};

// prototypes
struct mutex meme_mutex;
static int meme_open(struct inode* inode, struct file* file);
static int meme_release(struct inode* inode, struct file* file);
static ssize_t meme_read(struct file* file, char __user* buf, size_t size, loff_t* offset);
static ssize_t meme_write(struct file* file, const char __user* buf, size_t size, loff_t* offset);
static long meme_ioctl(struct file* file, unsigned int cmd, unsigned long arg);

// initialize file_operations
static const struct file_operations meme_fops =
{

	.owner			= THIS_MODULE,
	.open			= meme_open,
	.release		= meme_release,
	.read			= meme_read,
	.write			= meme_write,
	.unlocked_ioctl = meme_ioctl,

};

// Global Vars
uint64_t target = 0;

struct meme_device_data
{
	struct cdev cdev;
};

// global storage for device major number
static int dev_major = 0;

// sysfs class structure
static struct class *meme_class = NULL;

// array of meme_device_data for
static struct meme_device_data meme_data[MAX_DEV];

// sets permissions to all for user to interact with driver
static int meme_uevent(struct device* dev, struct kobj_uevent_env* env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}


// initialization function
static int __init meme_start(void)
{
	int err;
	dev_t dev;

	// allocate chardev region and assign major number
	err = alloc_chrdev_region(&dev, 0, MAX_DEV, "meme");
	if (err) 
	{
		pr_err("Failed to allocate chardev region\n");
		return err;
	}
	
	dev_major = MAJOR(dev);

	// create sysfs class
	meme_class = class_create(THIS_MODULE, "meme");
	if (IS_ERR(meme_class))
	{
		pr_err("Failed to create class\n");
		unregister_chrdev_region(dev, MAX_DEV);
		return PTR_ERR(meme_class);
	}
	
	meme_class->dev_uevent = meme_uevent;

    	cdev_init(&meme_data->cdev, &meme_fops);
    	meme_data->cdev.owner = THIS_MODULE;

	

    	err = cdev_add(&meme_data->cdev, MKDEV(dev_major, 1), 1);
   	if (err) 
	{
		pr_err("Failed to add cdev\n");
		class_destroy(meme_class);
		unregister_chrdev_region(dev, MAX_DEV);
		return err;
	}

    	device_create(meme_class, NULL, MKDEV(dev_major, 1), NULL, "meme");
    	mutex_init(&meme_mutex);

	return 0;
}


// cleanup function
static void __exit meme_end(void)
{
    mutex_destroy(&meme_mutex);
    device_destroy(meme_class, MKDEV(dev_major, 1));

	class_unregister(meme_class);
	class_destroy(meme_class);

	unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
}

static int meme_open(struct inode* inode, struct file* file)
{
    	pr_info("Hello World!");
    	mutex_lock(&meme_mutex);
	
	return 0;
}

static int meme_release(struct inode* inode, struct file* file)
{
    	pr_info("Device Released\n");
    	mutex_unlock(&meme_mutex);
	
	return 0;
}

static ssize_t meme_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
	pr_info("Device Read Called\n");
    if (size < sizeof(target))
        return -EINVAL;  // Ensure the buffer is large enough

    if (copy_to_user(buf, &target, sizeof(target)))
        return -EFAULT;  // Error copying to user space

	return sizeof(target);
}

static ssize_t meme_write(struct file* file, const char __user* buf, size_t size, loff_t* offset)
{
	pr_info("Device Write Called\n");

	return size;
}

static int meme_increment(struct meme_increment_t __user *arg)
{
	struct meme_increment_t increment;
	if (copy_from_user(&increment, arg, sizeof(increment)))
		return -EFAULT;
	
	target += increment.target;
	
	return 0;
}

static long meme_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	
	long ret = -ENOIOCTLCMD;

	switch (cmd) 
	{
	    case IOCTL_MEME_INCREMENT:
		pr_info("Meme increment ioctl called\n");
		
		ret = meme_increment((struct meme_increment_t *) arg);
		
        break;

	default:

		pr_err("Unable to handle ioctl %u\n", cmd);
		break;
	}

	return ret;
}

module_init(meme_start);
module_exit(meme_end);
