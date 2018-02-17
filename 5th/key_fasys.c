#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/poll.h>



#include <linux/gpio.h>

#include <mach/regs-gpio.h>
#include <mach/hardware.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>



static struct class* sec_class;

/*
 *读出中断值
 */
struct pin_desc{
	unsigned int pin;
	unsigned char key_val;
};

/*按键按下 01，02，03，04*/

struct pin_desc pins_desc[4]={
	
	{S3C2410_GPG0_EINT8 , 0x01},
	{S3C2410_GPG3_EINT11, 0x02},
	{S3C2410_GPG5_EINT13, 0x03},
	{S3C2410_GPG6_EINT14, 0x04},
};

//等待队列头
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static unsigned char key_vals;
//中断事件标志，中断将其置1，read清0
static volatile int evpress=0;

static struct fasync_struct *button_async;


static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	//printk("----------%s------------\n",__FUNCTION__);

	struct pin_desc* pindesc=(struct pin_desc*) dev_id;

	unsigned int pinval;
	
	pinval=s3c2410_gpio_getpin(pindesc->pin);


//低电平触发
	if (pinval)
	{
		/* 松开 */
		key_vals = 0x80 | pindesc->key_val;
	}
	else
	{
		/* 按下 */
		key_vals = pindesc->key_val;
	}


		/*唤醒进程*/
	evpress=1;
	wake_up_interruptible(&button_waitq);
	
	kill_fasync (&button_async, SIGIO, POLL_IN);

	return IRQ_HANDLED;

}

static int sec_drv_open(struct inode *inode,struct file *file)
{

	/*申请中断*/
	request_irq(IRQ_EINT8, buttons_irq,IRQF_TRIGGER_LOW,"S1",&pins_desc[0]);
	request_irq(IRQ_EINT11,buttons_irq,IRQF_TRIGGER_LOW,"S2",&pins_desc[1]);
	request_irq(IRQ_EINT13,buttons_irq,IRQF_TRIGGER_LOW,"S3",&pins_desc[2]);
	request_irq(IRQ_EINT14,buttons_irq,IRQF_TRIGGER_LOW,"S4",&pins_desc[3]);

   

	return 0;
}

static ssize_t sec_drv_read(struct file* file,const  char __user *buf, size_t count,loff_t * ppos)
{
	/*如果没有有按键发生，进入休眠 evpress=0*/
	wait_event_interruptible(button_waitq, evpress);
	
	/*如果有按键，返回键值,清零evpress*/
	copy_to_user(buf,&key_vals,1);
	
	evpress=0;

	return 1;
}

static int sec_drv_close (struct inode *inode, struct file *file )
{
	free_irq(IRQ_EINT8, &pins_desc[0]);
	free_irq(IRQ_EINT11,&pins_desc[1]);
	free_irq(IRQ_EINT13,&pins_desc[2]);
	free_irq(IRQ_EINT14,&pins_desc[3]);


	return 0;

}


static unsigned int sec_drv_poll (struct file * file,poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &button_waitq, wait); // 不会立即休眠

	if (evpress)
		mask |= POLLIN | POLLRDNORM;

	return mask;

}


static int sec_drv_fasync (int fd, struct file * file , int on)
{

	printk("----------%s------------\n",__FUNCTION__);

	return fasync_helper (fd, file, on, &button_async);


}


static struct file_operations sec_drv_ops={

.owner =THIS_MODULE,
.read  = sec_drv_read,
.open  = sec_drv_open,
.release=sec_drv_close,
.poll 	=sec_drv_poll,
.fasync=sec_drv_fasync,
};

int major;
int sec_drv_init(void)
{
	printk("----------%s------------\n",__FUNCTION__);

	major=register_chrdev(0, "secdrv",&sec_drv_ops);

	sec_class=class_create(THIS_MODULE, "sectdrv");
//class_device_create  内核升级替换为 device_create
	device_create(sec_class,NULL,MKDEV(major, 0),NULL,"buttons");
//	gpgcon=ioremap(0x56000060, 16);
//	gpgdat=gpgcon+1;

	return 0;
}
void sec_drv_exit(void)
{
	printk("----------%s------------\n",__FUNCTION__);

	unregister_chrdev(major, "secdrv");
	device_destroy(sec_class, MKDEV(major, 0));
	class_destroy(sec_class);
//	iounmap(0x56000060);
}



module_init(sec_drv_init);
module_exit(sec_drv_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("qifei");




