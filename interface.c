#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cryptctl.h"

int fd;
char *e_name = "/dev/cryptEncryptXX";
char *d_name = "/dev/cryptDecryptXX";

void create_driver(char* key){
    ioctl(fd, CTL_CREATE_DRIVER, key);
}

void delete_driver(int driver_id){
    ioctl(fd, CTL_DELETE_DRIVER, &driver_id);
}

void change_key(int driver_id, char* key){
    id_key driver;
    driver.id = driver_id;
    driver.key_length = (unsigned int) strlen(key);
    if(driver.key_length > 255){
        printf("Error: Key too large\n");
        return;
    }
    strcpy(driver.key, key);

    ioctl(fd, CTL_CHANGE_KEY, &driver);
}

void crypt(int driver_id, char* msg){
    read(fd, msg, strlen(msg));
}

int main(int argc, char** argv){
    if(argc < 3 || argc > 5){
        printf("Error: Invalid arguments\n");
        return -1;
    }

    // first argument is the function to be called
    // 0 - create driver
    // 1 - delete driver
    // 2 - change key, order is id then key
    // 3 - decrypt message
    // 4 - encrypt message
    if(argv[1][0] == '0'){  // create driver
        if(argc != 3){
            printf("Error: Invalid arguments\n");
            return -1;
        }
        char *key = argv[2];
        fd = open("/dev/cryptctl", O_RDWR);
        create_driver(key);
        close(fd);
        printf("Driver pair created with key: %s\n", key);
    }  
    else if(argv[1][0] == '1'){ // delete driver
        if(argc != 3){
            printf("Error: Invalid arguments\n");
            return -1;
        }
        int id = atoi(argv[2]);

        if(id < 0){
            printf("Error: Invalid arguments\n");
            return -1;
        }

        fd = open("/dev/cryptctl", O_RDWR);
        delete_driver(id);
        close(fd);

        printf("Driver pair %d deleted\n", id);
    }
    else if(argv[1][0] == '2'){ // change key
        if(argc != 4){
            printf("Error: Invalid arguments\n");
            return -1;
        }
        int id = atoi(argv[2]);
        char *key = argv[3];

        if(id < 0){
            printf("Error: Invalid arguments\n");
            return -1;
        }

        fd = open("/dev/cryptctl", O_RDWR);
        change_key(id, key);
        close(fd);
        printf("Driver pair %d key changed to: %s\n", id, key);
    }
    else if(argv[1][0] == '3' || argv[1][0] == '4'){
        // encrypt or decrypt
        if(argc != 4){ 
            printf("Error: Invalid arguments\n");
            return -1;
        }
        int id = atoi(argv[2]);
        char *msg = argv[3];

        if(id < 0){
            printf("Error: Invalid arguments\n");
            return -1;
        }

        char deviceID[3];
        sprintf(deviceID, "%d%d", id / 10, id % 10);
        if(argv[1][0] == '3'){
            // encrypt
            e_name[17] = deviceID[0];
            e_name[18] = deviceID[1];
            fd = open(e_name, O_RDWR);
            printf("%s encrypted to: ", msg);
        }
        else{
            // decrypt
            d_name[17] = deviceID[0];
            d_name[18] = deviceID[2];
            fd = open(d_name, O_RDWR);
            printf("%s decrypted to: ", msg);
        }

        crypt(id, msg);
        printf("%s\n", msg);
        close(fd);
    }

    return 0;
}