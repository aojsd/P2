#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include "cryptctl.h"              // allows dynamic major number to be shared

dev_t main_dev;
struct cdev cryptctl;
struct class *CryptClass;
int crypt_major;
int ctlOpen = 0;
id_key *change;
char key[KEY_MAX];

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Wu");
MODULE_DESCRIPTION("OS Assignment 2: Encrypt/Decrypt Pseudo-Device Driver");

int ctl_open(struct inode *inode, struct file *filp){
    if(ctlOpen > 0);
    ctlOpen++;
    return 0;
}

int ctl_release(struct inode *inode, struct file *filp){
    ctlOpen--;
    return 0;
}

long create_driver(char* key){
    return 0;
}

long delete_driver(int ID){
    return 0;
}

long change_key(id_key *change){
    return 0;
}

long ctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    int del;
    switch(cmd){
        case CTL_CREATE_DRIVER: // parameter is key, return driver ID
            copy_from_user(key, (char*)arg, KEY_MAX);
            return create_driver(key);
        case CTL_DELETE_DRIVER: // parameter is pointer to driver ID to delete
            get_user(del, (int*) arg);
            return delete_driver(del);
        case CTL_CHANGE_KEY: // parameter is pointer to id_key struct
            copy_from_user(change, (id_key*) arg, sizeof(id_key));
            return change_key(change);
    }
    return -1;
}

// define file operations
struct file_operations cryptctl_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = ctl_ioctl,
    .open = ctl_open,
    .release = ctl_release,
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
    if(device_create(CryptClass, NULL, main_dev, NULL, "cryptctl") == NULL){
        printk(KERN_NOTICE "Cryptctl: Error creating device\n");
        goto ERR_DEVICE_1;
    }

    // add cryptctl as a device driver
    cdev_init(&cryptctl, &cryptctl_fops);
    cryptctl.owner = THIS_MODULE;
    int err = cdev_add(&cryptctl, main_dev, 1);
    if(err < 0){
        printk(KERN_DEBUG "Cryptctl: Error adding cryptctl\n");
        goto ERR_DEVICE_2;
    }

    printk(KERN_DEBUG "Cryptctl: Initialized correctly with major number %d\n", crypt_major);
    return 0;

    ERR_DEVICE_2:
        cdev_del(&cryptctl);
        device_destroy(CryptClass, main_dev);
    ERR_DEVICE_1:
        class_unregister(CryptClass);
        class_destroy(CryptClass);
    ERR_CLASS:
        unregister_chrdev_region(main_dev, 1);
    ERR_MAJORNUM:
        return -1;
}

static void __exit cryptctl_exit(void){
    cdev_del(&cryptctl);
    device_destroy(CryptClass, main_dev);
    class_unregister(CryptClass);
    class_destroy(CryptClass);
    unregister_chrdev_region(main_dev, 1);
    printk(KERN_DEBUG "Exit Success\n");
}

module_init(cryptctl_init);
module_exit(cryptctl_exit);