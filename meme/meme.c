/**
 * @author Jered Wiegel
 * @date 18 Dec 2021
 * @version 0.6
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
MODULE_INFO(version, "0.6");

#define MAX_DEV 1
#define BUF_LEN 80
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)




// prototypes
static int meme_open(struct inode* inode, struct file* file);
static int meme_release(struct inode* inode, struct file* file);
static ssize_t meme_read(struct file* file, char __user* buf, size_t SIZE, loff_t* offset);
static ssize_t meme_write(struct file* file, const char __user* buf, size_t SIZE, loff_t* offset);
static long meme_ioctl(struct file* file, unsigned int cmd, unsigned long arg);

// initialize file_operations
static const struct file_operations meme_fops = {

	.owner			= THIS_MODULE,
	.open			= meme_open,
	.release		= meme_release,
	.read			= meme_read,
	.write			= meme_write,
	.unlocked_ioctl = meme_ioctl,

};


// Global Vars
static char msg[BUF_LEN];
static char* msg_Ptr;
static int Device_Open = 0;
int32_t value = 0;

struct meme_device_data {
	struct cdev cdev;
};

// global storage for device major number
static int dev_major = 0;

// sysfs class structure
static struct class *meme_class = NULL;

// array of meme_device_data for
static struct meme_device_data meme_data[MAX_DEV];

// sets permissions to all for user to interact with driver
static int meme_uevent(struct device* dev, struct kobj_uevent_env* env) {

	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}


// initialization function
static int __init meme_start(void) { 
	
	int err, i;
	dev_t dev;

	dev_major = MAJOR(dev);

	// allocate chardev region and assign major number
	err = alloc_chrdev_region(&dev, 0, MAX_DEV, "meme");

	// create sysfs class
	meme_class = class_create(THIS_MODULE, "meme");
	meme_class->dev_uevent = meme_uevent;


	// in order to allow for multiple devices to be created (increment the MAX_DEV var to inc/dec)
	for (i = 0; i < MAX_DEV; i++) {
		cdev_init(&meme_data[i].cdev, &meme_fops);
		meme_data[i].cdev.owner = THIS_MODULE;

		cdev_add(&meme_data[i].cdev, MKDEV(dev_major, i), 1);

		device_create(meme_class, NULL, MKDEV(dev_major, i), NULL, "meme-%d", i);
	}

	return 0;
}


// cleanup function
static void __exit meme_end(void) {
	
	int i;

	for (i = 0; i < MAX_DEV; i++) {
		device_destroy(meme_class, MKDEV(dev_major, i));
	}

	class_unregister(meme_class);
	class_destroy(meme_class);

	unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
}

// Open Function
static int meme_open(struct inode* inode, struct file* file) {
	printk("Device Opened\n");

	if (Device_Open) return -EBUSY;

	Device_Open++;
	sprintf(msg, "Hello world!\n");
	msg_Ptr = msg;
	

	return 0;
}

// Release Function
static int meme_release(struct inode* inode, struct file* file) {
	printk("Device Released\n");

	Device_Open--;

	return 0;
}
// Read Function
static ssize_t meme_read(struct file* file, char __user* buf, size_t SIZE, loff_t* offset) {
	printk("Device Read Called\n");

	int bytes_read = 0;

	if (msg_Ptr == 0) { 
		return 0; 
	}

	while (SIZE && *msg_Ptr) {

		put_user(*(msg_Ptr++), buf++);
		SIZE--;
		bytes_read++;
	}
	printk("bytes read: %d", bytes_read); // debug
	

	return msg;
}

// Write Function
static ssize_t meme_write(struct file* file, const char __user* buf, size_t SIZE, loff_t* offset) {
	printk("Device Write Called\n");

	printk("Function not supported by this driver.");
	return -EINVAL;
}

// Ioctl Function
static long meme_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case WR_VALUE:
		if (copy_from_user(&value, (int32_t*)arg, sizeof(value))) { 
		
			pr_err("Data Write : Err!\n");
		}

		pr_info("Value = %d\n", value++);

		break;

	case RD_VALUE:
		if (copy_to_user((int32_t*)arg, &value, sizeof(value))) {

			pr_err("Data Read : Err!\n");
		}
		break;

	default:

		pr_info("Default\n");
		

		break;
	}

	return 0;
}

module_init(meme_start);
module_exit(meme_end);
