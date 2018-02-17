#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <asm/uaccess.h>


#define NAME "at24xx"
#define at24xx_major 123

struct i2c_client * at24xx_client;
static struct class* at24xx_class;


static int at24xx_open (struct inode * inode, struct file * file)
{
	printk("--------%s----\n",__FUNCTION__);

	 return 0;
}


static ssize_t at24xx_read (struct file * file, char __user * buf, size_t count, loff_t *loff_t)
{
	printk("--------%s----\n",__FUNCTION__);

	unsigned char addr;
	unsigned char data;

	if(copy_from_user(&addr, buf, 1))
		return -1;

	data=i2c_smbus_read_byte_data(at24xx_client,addr);

	if(copy_to_user(&buf[1],&data,1))
		return -1;
	
	return 0;

}

static ssize_t at24xx_write (struct file * file ,const char __user * buf, size_t count, loff_t *loff_t)
{
	printk("--------%s----\n",__FUNCTION__);
	unsigned char at24_buf[2];
	unsigned char addr;
	unsigned char data;
	int err;

	if(copy_from_user(at24_buf, buf, sizeof(at24_buf)))
		return -1;

	addr=at24_buf[0];
	data=at24_buf[1];

	err=i2c_smbus_write_byte_data(at24xx_client,addr, data);
	if(err<0)
		return -ENODEV;

	
	return 0;

}

static struct file_operations at24xx_fileop={

	.owner=THIS_MODULE,
	.open=at24xx_open,
	.read=at24xx_read,
	.write=at24xx_write,
	
};

static int at24xx_probe(struct i2c_client * client, const struct i2c_device_id * id)
{
	printk("--------%s----\n",__FUNCTION__);

	at24xx_client=client;
	
	if (register_chrdev(at24xx_major, NAME, &at24xx_fileop)) {
		printk(KERN_ERR "adb: unable to get major %d\n", at24xx_major);
		return -ENODEV;
	}

	at24xx_class = class_create(THIS_MODULE, "at24xx");
	if (IS_ERR(at24xx_class))
		return -ENODEV;
	device_create(at24xx_class, NULL, MKDEV(at24xx_major, 0), NULL, "at24xx");

	return 0;
}
static int at24xx_remove(struct i2c_client *client)
{
	printk("--------%s----\n",__FUNCTION__);
	unregister_chrdev(at24xx_major,NAME);
	class_destroy(at24xx_class);
	device_destroy(at24xx_class, MKDEV(at24xx_major, 0));
	return 0;
}


static const struct i2c_device_id at24xx_id_table[]={
	{"at24xx",0},
};


static struct i2c_driver at24xx_drv={
	.driver = {
		.name = "at24xx",
		.owner = THIS_MODULE,
	},

	.probe=at24xx_probe,
	.remove=at24xx_remove,
	.id_table=at24xx_id_table,
};
static int __init at24xx_drv_init(void)
{	
	printk("--------%s----\n",__FUNCTION__);


	int err;
	err=i2c_add_driver(&at24xx_drv);
	if(err<0){
		printk("i2c_add_driver :error\n");
	}

	return 0;
}


static void __exit at24xx_drv_exit(void)
{
	printk("--------%s----\n",__FUNCTION__);


	i2c_del_driver(&at24xx_drv);
}


module_init(at24xx_drv_init);
module_exit(at24xx_drv_exit);
MODULE_LICENSE("GPL");

