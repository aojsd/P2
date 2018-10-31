#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include "crypt.h"              // allows dynamic major number to be shared
#define DEVICE_NAME "cryptctl"

dev_t main_dev;
struct cdev cryptctl;
struct class *CryptClass;
struct device *cryptmain;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Wu");
MODULE_DESCRIPTION("OS Assignment 2: Encrypt/Decrypt Pseudo-Device Driver");

// define file operations
struct file_operations crypt_fops = {
    //.owner = THIS_MODULE,
    //.read = crypt_read,
    //.write = crypt_write,
    //.ioctl = crypt_ioctl,
    //.open = crypt_open,
    //.release = crypt_release,
};

static int __init cryptctl_init(void){
    // dynamically allocate the major number, cryptctl will have minor number 0
    int result = alloc_chrdev_region(&main_dev, 0, 1, "cryptctl");
    crypt_major = MAJOR(main_dev);
    if(result < 0){
        printk(KERN_NOTICE "Cryptctl: Error allocating major number\n");
        goto ERR_MAJORNUM;
    }

    // create device class
    CryptClass = class_create(THIS_MODULE, "CS416 P2");
    if(IS_ERR(CryptClass)){
        printk(KERN_NOTICE "Cryptctl: Error creating class\n");
        goto ERR_CLASS;
    }

    // create device
    cryptmain = device_create(CryptClass, NULL, main_dev, NULL, "cryptctl");
    if(IS_ERR(cryptmain)){
        printk(KERN_NOTICE "Cryptctl: Error creating device\n");
        goto ERR_DEVICE_1;
    }

    // add cryptctl as a device driver
    cdev_init(&cryptctl, &crypt_fops);
    cryptctl->owner = THIS_MODULE;
    int err = cdev_add(&cryptctl, main_dev, 1);
    if(err < 0){
        printk(KERN_DEBUG "Cryptctl: Error adding cryptctl\n");
        goto ERR_DEVICE_2;
    }

    printk(KERN_DEBUG "Cryptctl: Initialized correctly.\n");
    return 0;

    ERR_DEVICE_2:
        cdev_del(&cryptctl);
        device_destroy(cryptmain);
    ERR_DEVICE_1:
        class_unregister(CryptClass);
        class_destroy(CryptClass);
    ERR_CLASS:
        unregister_chrdev_region(main_dev, 1);
    ERR_MAJORNUM:
        return -1;
}

static int __exit cryptctl_exit(void){
    cdev_del(&cryptctl);
    device_destroy(cryptmain);
    class_unregister(CryptClass);
    class_destroy(CryptClass);
    unregister_chrdev_region(main_dev, 1);
    printk(KERN_DEBUG "Exit Success\n");
    return 0;
}

module_init(cryptctl_init);
module_exit(cryptctl_exit);
