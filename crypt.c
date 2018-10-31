#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <crypt.h>              // allows dynamic major number to be shared
#define DEVICE_NAME "cryptctl"

dev_t main_dev;
struct cdev cryptctl;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Wu");
MODULE_DESCRIPTION("OS Assignment 2: Encrypt/Decrypt Pseudo-Device Driver");

// define file operations
struct file_operations crypt_fops = {
    .owner = THIS_MODULE;
    .read = crypt_read;
    .write = crypt_write;
    .ioctl = crypt_ioctl;
    .open = crypt_open;
    .release = crypt_release;
}

static int _ _init cryptctl_init(void){
    // dynamically allocate the major number, cryptctl will have minor number 0
    int result = alloc_chrdev_region(&main_dev, 0, 1, "cryptctl");
    crypt_major = MAJOR(main_dev);
    if(result < 0){
        return result;
    }

    // add cryptctl as a device driver
    cdev_init(&cryptctl, &crypt_fops);
    cryptctl->owner = THIS_MODULE;
    int err = cdev_add(&cryptctl, main_dev, 1);
    if(err){
        printk(KERN_DEBUG "Error adding cryptctl\n");
        cdev_del(&cryptctl);
        unregister_chrdev_region(main_dev, 1);
        return -1;
    }
    printk(KERN_DEBUG "Cryptctl initialized correctly.\n");
    return 0;
}

static int _ _exit cryptctl_exit(void){
    cdev_del(&cryptctl);
    unregister_chrdev_region(main_dev, 1);
    printk(KERN_DEBUG "Exit Success\n");
}

module_init(cryptctl_init);
module_exit(cryptctl_exit);
