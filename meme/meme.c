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
static int meme_open(struct inode* inode, struct file* file);
static int meme_release(struct inode* inode, struct file* file);
static ssize_t meme_read(struct file* file, char __user* buf, size_t size, loff_t* offset);
static ssize_t meme_write(struct file* file, const char __user* buf, size_t size, loff_t* offset);
static long meme_ioctl(struct file* file, unsigned int cmd, unsigned long arg);

// initialize file_operations
static const struct file_operations meme_fops
{

	.owner			= THIS_MODULE,
	.open			= meme_open,
	.release		= meme_release,
	.read			= meme_read,
	.write			= meme_write,
	.unlocked_ioctl = meme_ioctl,

};

// Global Vars
static char msg[BUF_LEN];
static char* msg_ptr;
static int device_open = 0;
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
	int err, i;
	dev_t dev;

	dev_major = MAJOR(dev);

	// allocate chardev region and assign major number
	err = alloc_chrdev_region(&dev, 0, MAX_DEV, "meme");

	// create sysfs class
	meme_class = class_create(THIS_MODULE, "meme");
	meme_class->dev_uevent = meme_uevent;

    cdev_init(&meme_data.cdev, &meme_fops);
    meme_data.cdev.owner = THIS_MODULE;

    cdev_add(&meme_data.cdev, MKDEV(dev_major), 1);
    device_create(meme_class, NULL, MKDEV(dev_major), NULL, "meme");


	return 0;
}


// cleanup function
static void __exit meme_end(void)
{
	device_destroy(meme_class, MKDEV(dev_major));

	class_unregister(meme_class);
	class_destroy(meme_class);

	unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
}

// Open Function
static int meme_open(struct inode* inode, struct file* file)
{

    pr_info("Device Opened\n");

	return 0;
}

// Release Function
static int meme_release(struct inode* inode, struct file* file)
{

    pr_info("Device Released\n");

	return 0;
}
// Read Function
static ssize_t meme_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
	pr_info("Device Read Called\n");

	int bytes_read;
	bytes_read = 0;

	if (msg_ptr == 0) {
		return 0;
	}

	while (size && *msg_ptr) {

		put_user(*(msg_ptr++), buf++);
		size--;
		bytes_read++;
	}
	pr_info("bytes read: %d", bytes_read); // debug

	return bytes_read;
}

// Write Function
static ssize_t meme_write(struct file* file, const char __user* buf, size_t size, loff_t* offset)
{
	pr_info("Device Write Called\n");

	return size;
}

static int meme_increment(struct meme_increment_t __user *arg)
{
	arg->target++;

	return 0;
}

// Ioctl Function
static long meme_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	
	long ret = -ENOIOCTLCMD;

	switch (cmd) {
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