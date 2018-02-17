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

static struct class* sec_class;

static volatile unsigned long *gpgcon=NULL;
static volatile unsigned long *gpgdat=NULL;


static int sec_drv_open(struct inode *inode,struct file *file)
{
   /*配置GPIOG为输入引脚*/
	*gpgcon&=~((0x03<<0)|(0x03<<6)|(0x03<<10)|(0x03<<12)|(0x03<<14)|(0x03<<22));
   

	return 0;
}

static ssize_t sec_drv_read(struct file* file,const  char __user *buf, size_t count,loff_t * ppos)
{
	/*返回6个引脚的电平*/
	unsigned char  key_val[6];
	int regval;

	if(count!=sizeof(key_val))
		return -EINVAL;
	regval=*gpgdat;

	key_val[0]=(regval&(1<<0))?1:0;
	key_val[1]=(regval&(1<<6))?1:0;
	key_val[2]=(regval&(1<<10))?1:0;
	key_val[3]=(regval&(1<<12))?1:0;
	key_val[4]=(regval&(1<<14))?1:0;
	key_val[5]=(regval&(1<<22))?1:0;

	
	copy_to_user(buf,key_val,sizeof(key_val));

	return sizeof(key_val);
}


static ssize_t sec_drv_write(struct file* file,const  char __user *buf, size_t count,loff_t * ppos)
{
	int val;
	copy_from_user(&val, buf,count);


	return 0;
}

static struct file_operations sec_drv_ops={

.owner =THIS_MODULE,
.write = sec_drv_write,
.read  = sec_drv_read,
.open  = sec_drv_open,

};

int major;
int sec_drv_init(void)
{
	printk("----------%s------------\n",__FUNCTION__);

	major=register_chrdev(0, "secdrv",&sec_drv_ops);

	sec_class=class_create(THIS_MODULE, "sectdrv");
//class_device_create  内核升级替换为 device_create
	device_create(sec_class,NULL,MKDEV(major, 0),NULL,"buttons");
	gpgcon=ioremap(0x56000060, 16);
	gpgdat=gpgcon+1;

	return 0;
}
void sec_drv_exit(void)
{
	printk("----------%s------------\n",__FUNCTION__);

	unregister_chrdev(major, "secdrv");
	device_destroy(sec_class, MKDEV(major, 0));
	class_destroy(sec_class);
	iounmap(0x56000060);
}



module_init(sec_drv_init);
module_exit(sec_drv_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("qifei");

