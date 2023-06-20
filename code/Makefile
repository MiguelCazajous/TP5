obj-m += gpio_module.o 

KDIR="/home/vagrant/linux"

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C ${KDIR} M=$(PWD) modules 

clean:
	make -C ${KDIR} M=$(PWD) clean

