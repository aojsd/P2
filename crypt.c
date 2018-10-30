#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#define DEVICE_NAME "cryptctl"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Wu");
MODULE_DESCRIPTION("OS Assignment 2: Encrypt/Decrypt Pseudo-Device Driver");

