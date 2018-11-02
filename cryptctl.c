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
#include <linux/list.h>
#include <linux/string.h>
#include "cryptctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Wu");
MODULE_DESCRIPTION("OS Assignment 2: Encrypt/Decrypt Pseudo-Device Driver");

typedef struct cryptPair{
    int id;
    unsigned int key_length;
    struct cdev encrypt;
    struct cdev decrypt;
    struct list_head plist;
    char key[KEY_MAX];
} c_pair;

dev_t main_dev;             // major and minor numbers of the ctl driver
struct cdev cryptctl;       // ctl driver
struct class *CryptClass;   // class for all drivers
int crypt_major;            // major number for all drivers
int ctlOpen = 0;            // checks whether cryptctl is open or not (basic synchronization)
int pair_ID = 0;            // moves to next pair ID when creating encrypt/decrypt pairs
LIST_HEAD(pair_list);       // kernel implementation of circular linked list

int pair_open(struct inode *inode, struct file *filp){
    // make it easier to find the key for encryption and decryption
    filp->private_data = container_of(inode->i_cdev, c_pair, cdev);
    return 0;
}

int pair_release(struct inode *inode, struct file *filp){
    return 0;
}

ssize_t encrypt(struct file *filp, char __user *buff, size_t count, loff_t *offp){
    // access device information
    c_pair *cpair = filp->private_data;

    // copy message to be encrypted from userspace
    char* msg = (char*)kmalloc((int) count, GFP_KERNEL);
    copy_from_user(msg, buff, count);

    // encrypt message
    int i; char keyChar;
    for(i = 0; i < count; i++){
        keyChar = cpair->key[i % cpair->key_length];
        msg[i] = ' ' + (msg[i] + keyChar) % LETTERS;
    }

    // copy message back into userspace
    copy_to_user(buff, msg, count);

    // free allocated memory
    kfree(msg);

    return count;
}

ssize_t decrypt(struct file *filp, char __user *buff, size_t count, loff_t *offp){
    c_pair *cpair = filp->private_data;

    return count;
}

// file operations for encryption drivers
struct file_operations e_fops = {
    .open = pair_open,
    .read = encrypt,
    .release = pair_release,
};

// file operations for decryption drivers
struct file_operations d_fops = {
    .open = pair_open,
    .read = decrypt,
    .release = pair_release,
};

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
    // dynamically allocate memory for driver pair
    c_pair *pair = (c_pair*) kmalloc(sizeof(c_pair), GFP_KERNEL);
    
    // set pair ID, key length, and key
    pair->id = pair_ID++;
    pair->key_length = (unsigned int)strlen(key);
    strcpy(pair->key, key);

    // create device drivers

    
    // init list_head and add to linked list
    INIT_LIST_HEAD(&pair->plist);
    list_add(&pair->plist, &pair_list);
    
    return 0;
}

long delete_driver(int ID){
    return 0;
}

long change_key(id_key *change){
    return 0;
}

long ctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    id_key change;
    char key[KEY_MAX];
    int del;

    switch(cmd){
        case CTL_CREATE_DRIVER: // parameter is key, return driver ID
            copy_from_user(key, (char*)arg, KEY_MAX);
            return create_driver(key);

        case CTL_DELETE_DRIVER: // parameter is pointer to driver ID to delete
            copy_from_user(&del, (int*) arg, sizeof(int));
            return delete_driver(del);

        case CTL_CHANGE_KEY: // parameter is pointer to id_key struct
            copy_from_user(&change, (id_key*) arg, sizeof(id_key));
            return change_key(&change);
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