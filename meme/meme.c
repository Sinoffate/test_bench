/**
 * @author Jered Wiegel
 * @date 18 Dec 2021
 * @version 0.2
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jered Wiegel");
MODULE_INFO(version, "0.2");

#define MAX_DEV 1

// prototypes
static int meme_open(struct inode* inode, struct file* file);
static int meme_release(struct inode* inode, struct file* file);
static ssize_t meme_read(struct file* file, char __user* buf, size_t count, loff_t* offset);

// initialize file_operations
static const struct file_operations meme_fops = {

	.owner		= THIS_MODULE,
	.open		= meme_open,
	.release	= meme_release,
	.read		= meme_read,

};


struct meme_device_data {
	struct cdev cdev;
};

// global storage for device major number
static int dev_major = 0;

// sysfs class structure
static struct class *meme_class = NULL;

// array of meme_device_data for
static struct meme_device_data meme_data[MAX_DEV];


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


static int meme_open(struct inode* inode, struct file* file) {
	return 0;
}


static int meme_release(struct inode* inode, struct file* file) {
	return 0;
}

static ssize_t meme_read(struct file* file, char __user* buf, size_t count, loff_t* offset) {

	uint8_t* data = "Hello world!\n";
	size_t datalen = strlen(data);

	printk("Reading device: %d\n", MINOR(file->f_path.dentry->d_inode->i_rdev));

	if (copy_to_user(buf, data, count)) {

		return -EFAULT;
	}

	if (*offset > 0) {

		return 0;
	}

	return datalen;
}

module_init(meme_start);
module_exit(meme_end);
