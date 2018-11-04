obj-m += cryptctl.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o interface interface.c
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf interface
