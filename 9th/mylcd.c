#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <mach/regs-lcd.h>
#include <mach/regs-gpio.h>
#include <mach/fb.h>

static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,	
							unsigned int green, unsigned int blue,  unsigned int transp, struct fb_info *info);
struct s3c_lcd_regs {
	volatile unsigned long 	lcdcon1;		
	volatile unsigned long 	lcdcon2;	
	volatile unsigned long 	lcdcon3; 	
	volatile unsigned long 	lcdcon4; 	
	volatile unsigned long 	lcdcon5; 	
	volatile unsigned long 	lcdsaddr1;
	volatile unsigned long 	lcdsaddr2;
	volatile unsigned long 	lcdsaddr3;
	volatile unsigned long 	redlut;		
	volatile unsigned long 	greenlut; 
	volatile unsigned long 	bluelut; 	
	volatile unsigned long 	dithmode; 
	volatile unsigned long 	reserve[9];
	volatile unsigned long 	tpal; 		
	volatile unsigned long 	lcdintpnd;
	volatile unsigned long 	lcdsrcpnd;
	volatile unsigned long 	lcdintmsk;
	volatile unsigned long 	tconsel;
};

static struct s3c_lcd_regs *lcd_regs;
static struct fb_info* s3c_lcd;
static volatile unsigned long *gpccon;
static volatile unsigned long *gpdcon;
static volatile unsigned long *gpgcon;
static unsigned long pseudo_palette[16];
static struct fb_ops s3c_fbops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= s3c_lcdfb_setcolreg,		/*	设置颜色,自己写*/
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static inline unsigned int chan_to_field(unsigned int chan, const struct fb_bitfield *bf){	chan &= 0xffff;	chan >>= 16 - bf->length;	return chan << bf->offset;}

static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,		/*	设置假的调色板,获取颜色值 */
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16)
		return 1;

	/* 用red,green,blue三原色构造出val */
	val  = chan_to_field(red,	&info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,	&info->var.blue);
	
	//((u32 *)(info->pseudo_palette))[regno] = val;
	pseudo_palette[regno] = val;
	return 0;
}

static int drv_23_lcd_init(void)
{
	s3c_lcd = framebuffer_alloc(0, NULL);

	/*	设置相关参数 */
	/*	1. 可变参数设置 */
	s3c_lcd->var.xres = 320;	//X方向分分辨率
	s3c_lcd->var.yres = 240;	//Y方向分分辨率
	s3c_lcd->var.xres_virtual = 320;	 //虚拟的分辨率
	s3c_lcd->var.yres_virtual = 240;
	s3c_lcd->var.bits_per_pixel = 32; 	//像素深度bpp，分配四个字节

	//RGB: 888 其中的一个字节丢弃掉，这个要看CPU的设置，可以高位也可以最低位
	s3c_lcd->var.red.offset = 16;
	s3c_lcd->var.red.length = 8;

	s3c_lcd->var.green.offset = 8;
	s3c_lcd->var.green.length = 8;

	s3c_lcd->var.blue.offset  = 0;
	s3c_lcd->var.blue.length = 8;

	s3c_lcd->var.activate       = FB_ACTIVATE_NOW;
	/*	2. 固定参数设置 */
	strcpy(s3c_lcd->fix.id, "LCD");
//	s3c_lcd->fix.smem_start = ;	/*	要设置*/
	s3c_lcd->fix.smem_len = 240 * 320 * 4;  //显存的大小
	s3c_lcd->fix.type     = FB_TYPE_PACKED_PIXELS;
	s3c_lcd->fix.visual = FB_VISUAL_TRUECOLOR;
	s3c_lcd->fix.line_length = 320 * 4;

	/*	3. 设置操作函数 */
	s3c_lcd->screen_size = 320*240*4;
	s3c_lcd->pseudo_palette = pseudo_palette;
	s3c_lcd->fbops = &s3c_fbops;
	/*	硬件操作 */
	/*	0. 设置引脚状态  */
	gpccon = (unsigned long*)ioremap(0x56000020, 4);
	*gpccon = 0xaaaaaaaa;

	gpdcon = (unsigned long*)ioremap(0x56000030, 4);
	*gpdcon = 0xaaaaaaaa;

	gpgcon = (unsigned long*)ioremap(0x56000060,4);		/*	设置为LCD_POWER  */
	*gpgcon = 3 << 8;

	/*	1. 设置LCD控制寄存器*/
	//最最重要的是这些寄存器的设置，学会查看LCD的时序图，学会调整，
	lcd_regs = (struct s3c_lcd_regs *)ioremap(0x4d000000, sizeof(lcd_regs));
	lcd_regs->lcdcon1 = (9 << 8) | (3 << 5) | (0xd << 1) | (0 << 0);
	lcd_regs->lcdcon2 = (11 << 24) | (239 << 14) | (16 << 6) | (0 << 0) ;
	lcd_regs->lcdcon3 = (7 << 19) | (319 << 8) | (7 << 0);
	lcd_regs->lcdcon4 = (0 << 0);
	lcd_regs->lcdcon5 =  (1 << 10) | (1 << 9) | (1 << 8) | (0 << 1) | (0 << 0);

	/*	2. 设置LCD地址寄存器 */
	/*	分配显存地址 */
	s3c_lcd->screen_base = dma_alloc_writecombine(NULL, s3c_lcd->fix.smem_len, &s3c_lcd->fix.smem_start, GFP_KERNEL);	/*	先分配start,返回虚拟内存起始地址   */
	lcd_regs->lcdsaddr1 = (s3c_lcd->fix.smem_start >> 1) & (~(1 << 30));
	lcd_regs->lcdsaddr2 = ((s3c_lcd->fix.smem_start + s3c_lcd->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3 = (320*32/16);

	lcd_regs->lcdcon1 |= (1<<0); /* 使能LCD控制器 */
	lcd_regs->lcdcon5 |= (1<<3); /* 使能LCD本身: LCD_PWREN */
	/*	注册fb_info结构 */
	register_framebuffer(s3c_lcd);
	
	return 0;
}
	
static void drv_23_lcd_exit(void)
{	
	lcd_regs->lcdcon1 &= ~(1<<0); /* 关闭LCD控制器 */
	lcd_regs->lcdcon1 &= ~(1<<3); /* 关闭LCD本身 */

	unregister_framebuffer(s3c_lcd);
	dma_free_writecombine(NULL,  s3c_lcd->fix.smem_len, s3c_lcd->screen_base, s3c_lcd->fix.smem_start);
	iounmap(lcd_regs);
	iounmap(gpccon);
	iounmap(gpdcon);
	iounmap(gpgcon);
	framebuffer_release(s3c_lcd);
}

module_init(drv_23_lcd_init);
module_exit(drv_23_lcd_exit);
MODULE_LICENSE("GPL");

