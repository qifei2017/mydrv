#include<linux/module.h>
#include<linux/i2c.h>
#include<linux/kernel.h>


static struct i2c_board_info at24xx_board_info={
	I2C_BOARD_INFO("at24xx", 0x50),
};
static struct i2c_client *at24xx_client;
static int __init at24xx_dev_init(void)
{
	struct i2c_adapter * at24xx_adap;
/*获取i2c适配器*/
	at24xx_adap=i2c_get_adapter(0);
/*注册新的设备*/
	at24xx_client=i2c_new_device(at24xx_adap , &at24xx_board_info);
/*将设备放入 适配器链表*/
	i2c_put_adapter(at24xx_adap);

	return 0;
}


static void __exit at24xx_dev_exit(void)
{
	i2c_unregister_device(at24xx_client);
}


module_init(at24xx_dev_init);
module_exit(at24xx_dev_exit);
MODULE_LICENSE("GPL");


