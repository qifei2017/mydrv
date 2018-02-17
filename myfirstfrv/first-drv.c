#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>

static struct class* first_class;

static volatile unsigned long *gpbcon=NULL;
static volatile unsigned long *gpbdat=NULL;


static int first_drv_open(struct inode *inode,struct file *file)
{
	printk("open first driver\n");
	*gpbcon&=~((0x03<<10)|(0x03<<12)|(0x03<<14)|(0x03<<16));
	*gpbcon|=((0x01<<10)|(0x01<<12)|(0x01<<14)|(0x01<<16));

	return 0;
}

static ssize_t first_drv_write(struct file* file,const  char __user *buf, size_t count,loff_t * ppos)
{
	int val;
	copy_from_user(&val, buf,count);
	if(val==1)
		{
		*gpbdat|=((1<<5)|(1<<6)|(1<<7)|(1<<8));
	}
	else{

		*gpbdat&=~((1<<5)|(1<<6)|(1<<7)|(1<<8));

	}
	return 0;
}

/*注意位置，否则将会出现未定义的错误*/
static struct file_operations first_drv_ops={

.owner =THIS_MODULE,
.write = first_drv_write,
.open =first_drv_open,

};

int major;
int first_drv_init(void)
{
	printk("----------%s------------\n",__FUNCTION__);
	
	major=register_chrdev(0,"first_drv",&first_drv_ops);

	first_class=class_create(THIS_MODULE, "firstdrv");
//class_device_create  内核升级替换为 device_create
	device_create(first_class,NULL,MKDEV(major, 0),NULL,"first");

	gpbcon=(volatile unsigned long *)ioremap(0x56000010, 16);
	gpbdat=gpbcon+1;

	return 0;
}

void first_drv_exit(void)
{
	printk("----------%s------------\n",__FUNCTION__);

	*gpbdat&=~((1<<5)|(1<<6)|(1<<7)|(1<<8));

	unregister_chrdev(major,"first_drv");
	device_destroy(first_class, MKDEV(major, 0));
	class_destroy(first_class);
	iounmap(0x56000010);
}


module_init(first_drv_init);
module_exit(first_drv_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("qifei");
