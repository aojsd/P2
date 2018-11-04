#ifndef _cryptctl_h
#define _cryptctl_h

#include <linux/ioctl.h>

#define KEY_MAX 256
#define LETTERS 95
#define encrypt_name "cryptEncrypt"
#define decrypt_name "cryptDecrypt"

typedef struct id_key{
    int id;
    unsigned int key_length;
    char key[KEY_MAX];
} id_key;

#define CTL_IOC_MAGIC 0xF9

#define CTL_CREATE_DRIVER _IOW(CTL_IOC_MAGIC, 1, char*)
#define CTL_DELETE_DRIVER _IOW(CTL_IOC_MAGIC, 2, int*)
#define CTL_CHANGE_KEY    _IOW(CTL_IOC_MAGIC, 3, id_key*)

#endif
