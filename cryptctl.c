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

// represents a pair of encrypt and decrypt drivers
typedef struct cryptPair{
    int id;
    unsigned int key_length;
    struct cdev dev_encrypt;
    struct cdev dev_decrypt;
    struct list_head plist;
    char key[KEY_MAX];
} c_pair;


// global variables
dev_t main_dev;             // major and minor numbers of the ctl driver
struct cdev cryptctl;       // ctl driver
struct class *CryptClass;   // class for all drivers
int crypt_major;            // major number for all drivers
int ctlOpen = 0;            // checks whether cryptctl is open or not (basic synchronization)
int pair_ID = 0;            // moves to next pair ID when creating encrypt/decrypt pairs
LIST_HEAD(pair_list);       // kernel implementation of circular linked list
char *e_name = "cryptEncryptXX";
char *d_name = "cryptDecryptXX";

// open function for encrypt drivers
int e_open(struct inode *inode, struct file *filp){
    // make it easier to find the key for encryption and decryption
    filp->private_data = container_of(inode->i_cdev, c_pair, dev_encrypt);
    return 0;
}

// open function for encrypt drivers
int d_open(struct inode *inode, struct file *filp){
    // make it easier to find the key for encryption and decryption
    filp->private_data = container_of(inode->i_cdev, c_pair, dev_decrypt);
    return 0;
}

// release function for both encrypt and decrypt drivers
int pair_release(struct inode *inode, struct file *filp){
    return 0;
}

// read function for encrypt drivers, takes message in buff and encrypts it in place
ssize_t encrypt(struct file *filp, char __user *buff, size_t count, loff_t *offp){
    size_t i;
    char keyChar;
    c_pair *cpair;
    char *msg;

    // access device information
    cpair = filp->private_data;

    // copy message to be encrypted from userspace
    msg = (char*)kmalloc((int) count, GFP_KERNEL);
    copy_from_user(msg, buff, count);

    // encrypt message
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

// read function for decrypt drivers, takes encrypted message in buff and decrypts it in place
ssize_t decrypt(struct file *filp, char __user *buff, size_t count, loff_t *offp){
    size_t i;
    char keyChar;
    c_pair *cpair;
    char *msg;
    
    // access device information
    cpair = filp->private_data;

    // copy message to be encrypted from userspace
    msg = (char*)kmalloc((int) count, GFP_KERNEL);
    copy_from_user(msg, buff, count);

    // decrypt message
    for(i = 0; i < count; i++){
        keyChar = cpair->key[i % cpair->key_length];
        msg[i] = (msg[i] - ' ' - keyChar + 2*LETTERS) % LETTERS;
    }

    // copy message back into userspace
    copy_to_user(buff, msg, count);

    // free allocated memory
    kfree(msg);

    return count;
}

// file operations for encryption drivers
struct file_operations e_fops = {
    .open = e_open,
    .read = encrypt,
    .release = pair_release,
};

// file operations for decryption drivers
struct file_operations d_fops = {
    .open = d_open,
    .read = decrypt,
    .release = pair_release,
};

// open function for cryptctl
int ctl_open(struct inode *inode, struct file *filp){
    return 0;
}

// release function for cryptctl
int ctl_release(struct inode *inode, struct file *filp){
    return 0;
}

// called by ioctl, creates encrypt and decrypt drivers with the given key
long create_driver(char* key){
    dev_t e_dev, d_dev;
    c_pair *pair;
    char *deviceID;

    // dynamically allocate memory for driver pair
    pair = (c_pair*) kmalloc(sizeof(c_pair), GFP_KERNEL);
    
    // set pair ID, key length, and key
    pair->id = pair_ID++;
    pair->key_length = (unsigned int)strlen(key);
    strcpy(pair->key, key);
    
    // create device drivers                  
    e_dev = MKDEV(crypt_major, 2*pair_ID - 1);  // make device major and minor numbers
    d_dev = MKDEV(crypt_major, 2*pair_ID);

    deviceID = "XX";                                  // make device names
    sprintf(deviceID, "%d", pair_ID);
    e_name[12] = deviceID[0]; e_name[13] = deviceID[1];
    d_name[12] = deviceID[0]; d_name[13] = deviceID[1];

    register_chrdev_region(e_dev, 1, e_name);       // register device numbers
    register_chrdev_region(d_dev, 1, d_name);
    
    device_create(CryptClass, NULL, e_dev, NULL, e_name);   // create device nodes
    device_create(CryptClass, NULL, d_dev, NULL, d_name);

    cdev_init(&pair->dev_encrypt, &e_fops);         // initialize character drivers
    cdev_init(&pair->dev_decrypt, &d_fops);

    cdev_add(&pair->dev_encrypt, e_dev, 1);         // make drivers visible to kernel
    cdev_add(&pair->dev_decrypt, d_dev, 1);

    // init list_head and add to linked list
    INIT_LIST_HEAD(&pair->plist);
    list_add(&pair->plist, &pair_list);
    
    return 0;
}

// called by ioctl, deletes driver pair of a given ID
long delete_driver(int ID){
    dev_t e_dev, d_dev;
    
    // search list for driver pair
    c_pair *pair = NULL;
    list_for_each_entry(pair, &pair_list, plist){
        if(pair->id == ID)
            break;
    }
    list_del(&pair->plist);    // remove pair from list

    // delete drivers from kernel
    e_dev = pair->dev_encrypt.dev;
    d_dev = pair->dev_decrypt.dev;

    cdev_del(&pair->dev_decrypt);    // delete cdev objects
    cdev_del(&pair->dev_encrypt);
    
    device_destroy(CryptClass, e_dev);    // delete device node
    device_destroy(CryptClass, d_dev);

    unregister_chrdev_region(e_dev, 1);    // unregister device numbers
    unregister_chrdev_region(d_dev, 1);

    // free memory of driver pair
    kfree(pair);

    return 0;
}

// called by ioctl, changes key of a driver pair given their id
long change_key(id_key *change){
    // search list for driver pair
    c_pair *pair = NULL;
    list_for_each_entry(pair, &pair_list, plist){
        if(pair->id == change->id)
            break;
    }

    // update key
    strcpy(pair->key, change->key);

    // update key_length
    pair->key_length = change->key_length;

    return 0;
}

// ioctl function for cryptctl
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
    CryptClass = class_create(THIS_MODULE, "Crypt");
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
    if(cdev_add(&cryptctl, main_dev, 1) < 0){
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

void cleanup(void){                 // delete any remaining device pairs
    c_pair *pair;
    dev_t e_dev, d_dev;
    
    // hit every item of list
    while(!list_empty(&pair_list)){
        pair = list_first_entry(&pair_list, c_pair, plist);

        list_del(&pair->plist);    // remove pair from list

        // delete drivers from kernel
        e_dev = pair->dev_encrypt.dev;
        d_dev = pair->dev_decrypt.dev;

        cdev_del(&pair->dev_decrypt);    // delete cdev objects
        cdev_del(&pair->dev_encrypt);
    
        device_destroy(CryptClass, e_dev);    // delete device node
        device_destroy(CryptClass, d_dev);

        unregister_chrdev_region(e_dev, 1);    // unregister device numbers
        unregister_chrdev_region(d_dev, 1);

        // free memory of driver pair
        kfree(pair);
    }
}

static void __exit cryptctl_exit(void){
    cleanup();
    cdev_del(&cryptctl);
    device_destroy(CryptClass, main_dev);
    class_unregister(CryptClass);
    class_destroy(CryptClass);
    unregister_chrdev_region(main_dev, 1);
    printk(KERN_DEBUG "Exit Success\n");
}

module_init(cryptctl_init);
module_exit(cryptctl_exit);