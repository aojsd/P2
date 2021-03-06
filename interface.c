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
char *e_name = "/dev/cryptEncrypt";
char *d_name = "/dev/cryptDecrypt";

long create_driver(char* key){
    return ioctl(fd, CTL_CREATE_DRIVER, key);
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
        long id;
        fd = open("/dev/cryptctl", O_RDWR);
        id = create_driver(key);
        close(fd);
        printf("Driver pair created\nID: %ld\nKey: %s\n", id, key);
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
	char filename[22] = "";

        if(id < 0){
            printf("Error: Invalid arguments\n");
            return -1;
        }

        char deviceID[3];
        sprintf(deviceID, "%d", id);
        if(argv[1][0] == '3'){
            // encrypt
            strcat(filename, e_name);
	    strcat(filename, deviceID);
            printf("%s encrypted as: ", msg);
        }
        else{
            // decrypt
            strcat(filename, d_name);
	    strcat(filename, deviceID);
            printf("%s decrypted to: ", msg);
	}
	fd = open(filename, O_RDWR);
        crypt(id, msg);
        printf("%s\n", msg);

        close(fd);
    }

    return 0;
}
