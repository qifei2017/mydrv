KERN_DIR=/home/mini2440/linux-2.6.32

all:
	make -C $(KERN_DIR) M=`pwd` modules
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
obj-m+=key_EINT.o

