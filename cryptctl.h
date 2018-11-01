#include <linux/ioctl.h>

#ifndef _cryptctl_h
#define _cryptctl_h

#define KEY_MAX 256
typedef struct id_key{
    int id;
    char key[KEY_MAX];
    int key_length;
} id_key;

#define CTL_IOC_MAGIC 0xF9

#define CTL_CREATE_DRIVER _IO(CTL_IOC_MAGIC, 1, char*)
#define CTL_DELETE_DRIVER _IO(CTL_IOC_MAGIC, 2, int)
#define CTL_CHANGE_KEY    _IOW(CTL_IOC_MAGIC, 3, id_key*)

#endif
