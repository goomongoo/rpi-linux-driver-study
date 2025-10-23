obj-m += hello.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules CC=/usr/bin/x86_64-linux-gnu-gcc-12

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean