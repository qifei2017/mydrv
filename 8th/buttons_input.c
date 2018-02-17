/*参考GPIO_KEYS.C*/
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
#include <linux/input.h>



#include <asm/gpio.h>

struct pin_desc {
	int irq;
	unsigned int pin;
	char * name;
	unsigned int key_val;
};
struct pin_desc pins_desc[4]={
	{IRQ_EINT8,S3C2410_GPG0_EINT8 , "S1", KEY_L},
	{IRQ_EINT11,S3C2410_GPG3_EINT11,"S2", KEY_S},
	{IRQ_EINT13,S3C2410_GPG5_EINT13,"S3", KEY_ENTER},
	{IRQ_EINT14,S3C2410_GPG6_EINT14,"S4", KEY_LEFTSHIFT},
};
static struct timer_list buttons_timer;
struct input_dev* buttons_dev;
struct pin_desc* irq_pd;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	/* 10ms后启动定时器 */
	irq_pd = (struct pin_desc *)dev_id;
	mod_timer(&buttons_timer, jiffies+HZ/100);
	return IRQ_RETVAL(IRQ_HANDLED);
}

static void do_buttons_timer(unsigned long data)
{
	
	struct pin_desc* pindesc=(struct pin_desc*) irq_pd;
	
	unsigned int pinval;
		
	pinval=s3c2410_gpio_getpin(pindesc->pin);
	
	
//低电平触发
	if (pinval)
	{
		/* 松开 */
		input_event(buttons_dev,EV_KEY, pindesc->key_val,0);
	}
	else
	{
		/* 按下 */
		input_event(buttons_dev,EV_KEY, pindesc->key_val,1);

	}

}

static int buttons_init(void)
{
/*1.分配一个input_dev 结构体*/
	int i;

	buttons_dev=input_allocate_device();
	
/*2.设置*/
	/*2.1设置能产生哪类事件*/
	set_bit(EV_KEY,buttons_dev->keybit);
	/*2.2能产生这类操作里面的哪些事件：l,s,enter，leftshit*/
	set_bit(KEY_L,buttons_dev->keybit);
	set_bit(KEY_S,buttons_dev->keybit);
	set_bit(KEY_ENTER,buttons_dev->keybit);
	set_bit(KEY_LEFTSHIFT,buttons_dev->keybit);

/*3.注册*/
	input_register_device(buttons_dev);

/*4.硬件相关操作*/
	init_timer(&buttons_timer);
	buttons_timer.function = do_buttons_timer;
	buttons_timer.expires = jiffies + 1;
	add_timer(&buttons_timer);

	for(i=0;i<4;i++)
		{
			request_irq(pins_desc[i].irq, buttons_irq,IRQF_TRIGGER_LOW,pins_desc[i].name,&pins_desc[i]);
		}
	return 0;
}
static void buttons_exit(void)
{
	int i;

	for(i=0;i<4;i++)
		{
			free_irq(pins_desc[i].irq,&pins_desc[i]);
		}
	del_timer(&buttons_timer);
	input_unregister_device(buttons_dev);
	input_free_device(buttons_dev);

}

module_init(buttons_init);
module_exit(buttons_exit);




