/**
 * @author Jered Wiegel
 * @date 18 Dec 2021
 * @version 0.1
 * 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

// initialization function
static int __init meme_start(void) { 
	printk(KERN_INFO "Hello world!\n");
	return 0;
}

// cleanup function
static void __exit meme_end(void) {
	printk(KERN_INFO "Cya Lata!");

}

module_init(meme_start);
module_exit(meme_end);
