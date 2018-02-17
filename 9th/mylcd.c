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
	.fb_setcolreg	= s3c_lcdfb_setcolreg,		/*	������ɫ,�Լ�д*/
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static inline unsigned int chan_to_field(unsigned int chan, const struct fb_bitfield *bf){	chan &= 0xffff;	chan >>= 16 - bf->length;	return chan << bf->offset;}

static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,		/*	���üٵĵ�ɫ��,��ȡ��ɫֵ */
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16)
		return 1;

	/* ��red,green,blue��ԭɫ�����val */
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

	/*	������ز��� */
	/*	1. �ɱ�������� */
	s3c_lcd->var.xres = 320;	//X����ֱַ���
	s3c_lcd->var.yres = 240;	//Y����ֱַ���
	s3c_lcd->var.xres_virtual = 320;	 //����ķֱ���
	s3c_lcd->var.yres_virtual = 240;
	s3c_lcd->var.bits_per_pixel = 32; 	//�������bpp�������ĸ��ֽ�

	//RGB: 888 ���е�һ���ֽڶ����������Ҫ��CPU�����ã����Ը�λҲ�������λ
	s3c_lcd->var.red.offset = 16;
	s3c_lcd->var.red.length = 8;

	s3c_lcd->var.green.offset = 8;
	s3c_lcd->var.green.length = 8;

	s3c_lcd->var.blue.offset  = 0;
	s3c_lcd->var.blue.length = 8;

	s3c_lcd->var.activate       = FB_ACTIVATE_NOW;
	/*	2. �̶��������� */
	strcpy(s3c_lcd->fix.id, "LCD");
//	s3c_lcd->fix.smem_start = ;	/*	Ҫ����*/
	s3c_lcd->fix.smem_len = 240 * 320 * 4;  //�Դ�Ĵ�С
	s3c_lcd->fix.type     = FB_TYPE_PACKED_PIXELS;
	s3c_lcd->fix.visual = FB_VISUAL_TRUECOLOR;
	s3c_lcd->fix.line_length = 320 * 4;

	/*	3. ���ò������� */
	s3c_lcd->screen_size = 320*240*4;
	s3c_lcd->pseudo_palette = pseudo_palette;
	s3c_lcd->fbops = &s3c_fbops;
	/*	Ӳ������ */
	/*	0. ��������״̬  */
	gpccon = (unsigned long*)ioremap(0x56000020, 4);
	*gpccon = 0xaaaaaaaa;

	gpdcon = (unsigned long*)ioremap(0x56000030, 4);
	*gpdcon = 0xaaaaaaaa;

	gpgcon = (unsigned long*)ioremap(0x56000060,4);		/*	����ΪLCD_POWER  */
	*gpgcon = 3 << 8;

	/*	1. ����LCD���ƼĴ���*/
	//������Ҫ������Щ�Ĵ��������ã�ѧ��鿴LCD��ʱ��ͼ��ѧ�������
	lcd_regs = (struct s3c_lcd_regs *)ioremap(0x4d000000, sizeof(lcd_regs));
	lcd_regs->lcdcon1 = (9 << 8) | (3 << 5) | (0xd << 1) | (0 << 0);
	lcd_regs->lcdcon2 = (11 << 24) | (239 << 14) | (16 << 6) | (0 << 0) ;
	lcd_regs->lcdcon3 = (7 << 19) | (319 << 8) | (7 << 0);
	lcd_regs->lcdcon4 = (0 << 0);
	lcd_regs->lcdcon5 =  (1 << 10) | (1 << 9) | (1 << 8) | (0 << 1) | (0 << 0);

	/*	2. ����LCD��ַ�Ĵ��� */
	/*	�����Դ��ַ */
	s3c_lcd->screen_base = dma_alloc_writecombine(NULL, s3c_lcd->fix.smem_len, &s3c_lcd->fix.smem_start, GFP_KERNEL);	/*	�ȷ���start,���������ڴ���ʼ��ַ   */
	lcd_regs->lcdsaddr1 = (s3c_lcd->fix.smem_start >> 1) & (~(1 << 30));
	lcd_regs->lcdsaddr2 = ((s3c_lcd->fix.smem_start + s3c_lcd->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3 = (320*32/16);

	lcd_regs->lcdcon1 |= (1<<0); /* ʹ��LCD������ */
	lcd_regs->lcdcon5 |= (1<<3); /* ʹ��LCD����: LCD_PWREN */
	/*	ע��fb_info�ṹ */
	register_framebuffer(s3c_lcd);
	
	return 0;
}
	
static void drv_23_lcd_exit(void)
{	
	lcd_regs->lcdcon1 &= ~(1<<0); /* �ر�LCD������ */
	lcd_regs->lcdcon1 &= ~(1<<3); /* �ر�LCD���� */

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

