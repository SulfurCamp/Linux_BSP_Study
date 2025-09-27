#include "linux/printk.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mutex.h>

#define DEBUG 1
#define GPIOCNT 8
#define LED_OFF 0

#define 	KERNELTIMER_NAME 	"kerneltimer"
#define		KERNELTIMER_MAJOR 	230

static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};
static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int irqKey[GPIOCNT];

static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);
static int gpioKeyInit(void);
//static int gpioKeyGet(void);
static void gpioKeyFree(void);

static int openFlag = 0;

static int timerVal = 100; //f=100Hz, T=1/100 = 10ms, 100 *10ms = 1sec
module_param(timerVal, int , 0);
static int ledVal = 0;
module_param(ledVal, int, 0);
struct timer_list timerLed;
static void kerneltimer_func(struct timer_list *t);
static int keyNum = 0;
static DEFINE_MUTEX(keyMutex);
///////////////////////////////////////////////////////////////////////////
static irqreturn_t keyIsr(int irq, void * data)
{
	int i;
	
	for(i=0;i<GPIOCNT;i++)
	{
		if(irq == irqKey[i])
		{
			if(mutex_trylock(&keyMutex) != 0)
			{
				keyNum = i + 1;
				mutex_unlock(&keyMutex);
				break;
			}
		}
	}
#if DEBUG
	printk("keyIsr() irq : %d, keyNum : %d\n",irq, keyNum);
#endif
	return IRQ_HANDLED;
}
///////////////////////////////////////////////////////////////////////////
static int gpioLedInit(void)
{
	int i;
	int ret=0 ;
	char gpioName[10];
	for(i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"led%d",i);
		ret=gpio_request(gpioLed[i],gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n",gpioLed[i]);
			return ret;
		}
	}
	for(i=0;i<GPIOCNT;i++)
	{
		ret = gpio_direction_output(gpioLed[i],LED_OFF);
		if(ret < 0)
		{
			printk("Failed direction_output gpio%d error\n",gpioLed[i]);
			return ret;
		}
	}
	return ret;
}
///////////////////////////////////////////////////////////////////////////
static void gpioLedSet(long val)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_set_value(gpioLed[i],((val >> i) & 0x01));
	}			
}
static void gpioLedFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}
///////////////////////////////////////////////////////////////////////////
static int gpioKeyInit(void)
{
	int i;
	int ret=0;
	char gpioName[10];
	for(i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"key%d",i);
		ret = gpio_request(gpioKey[i], gpioName);
		if(ret < 0) {
			printk("Failed Request gpio%d error\n", gpioKey[i]);
			return ret;
		}
		ret = gpio_direction_input(gpioKey[i]);
		if(ret < 0) {
			printk("Failed direction_output gpio%d error\n", gpioKey[i]);
       	 	return ret;
		}
	}
	return ret;
}
///////////////////////////////////////////////////////////////////////////
static int irqKeyInit(void)
{
	int i;
	int ret=0;
	for(i=0;i<GPIOCNT;i++)
	{
		irqKey[i] = gpio_to_irq(gpioKey[i]);
		if(irqKey[i] < 0)
		{
			printk("Failed gpio_to_irq() gpio%d error\n",gpioKey[i]);
			return irqKey[i];
		}
#if DEBUG
		else
			printk("gpio_to_irq() gpio%d (irq%d) \n",gpioKey[i],irqKey[i]);
#endif
	}
	return ret;
}
///////////////////////////////////////////////////////////////////////////
static void irqKeyFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		free_irq(irqKey[i], NULL);
	}
}
///////////////////////////////////////////////////////////////////////////
//static int	gpioKeyGet(void)
//{
//	int i;
//	int ret;
//	int keyData=0;
//	for(i=0;i<GPIOCNT;i++)
//	{
////		ret=gpio_get_value(gpioKey[i]) << i;
////		keyData |= ret;
//		ret=gpio_get_value(gpioKey[i]);
//		keyData = keyData | ( ret << i );
//	}
//	return keyData;
//}
///////////////////////////////////////////////////////////////////////////
static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}
///////////////////////////////////////////////////////////////////////////
static int kerneltimer_open (struct inode *inode, struct file *flip)
{

		int num0 = MAJOR(inode->i_rdev);
		int num1 = MINOR(inode->i_rdev);
 	    printk( "call open -> major : %d\n", num0 );
    	printk( "call open -> minor : %d\n", num1 );
		if(openFlag)
				return -EBUSY;
		else
				openFlag = 1;
		if(!try_module_get(THIS_MODULE))
				return -ENODEV;

		return 0;
}
///////////////////////////////////////////////////////////////////////////
static ssize_t kerneltimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf=0;
//	kbuf=gpioKeyGet();
	if(mutex_trylock(&keyMutex) != 0)
	{
		if(keyNum != 0)
		{
			kbuf=(char)keyNum;
			keyNum = 0;
		}
		mutex_unlock(&keyMutex);
	}
#if 1
	put_user(kbuf,buf);
#else
	int ret=copy_to_user(buf,&kbuf,sizeof(kbuf));
	if(ret < 0)
        return ret;
#endif
#if DEBUG
    //printk( "call read -> key : %#04x\n", kbuf );
#endif
    return sizeof(kbuf);
}
///////////////////////////////////////////////////////////////////////////
static ssize_t kerneltimer_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;

#if 1
	get_user(kbuf,buf);
#else
	int ret;
	ret=copy_from_user(&kbuf,buf,sizeof(kbuf));
#endif
#if DEBUG
    printk( "call write -> ledVal : %#04x\n", kbuf );
#endif
	ledVal = kbuf;
	//gpioLedSet(kbuf);
	return count;
}
///////////////////////////////////////////////////////////////////////////
static long kerneltimer_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
    return err;
}
///////////////////////////////////////////////////////////////////////////
static int kerneltimer_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );

	module_put(THIS_MODULE);
	openFlag = 0;
    return 0;
}
///////////////////////////////////////////////////////////////////////////
static void kerneltimer_registertimer(unsigned long timeover)
{
		timerLed.expires = get_jiffies_64() + timeover;
		timer_setup( &timerLed, kerneltimer_func, 0);
		add_timer( &timerLed);
}
///////////////////////////////////////////////////////////////////////////
static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
		printk("ledVal : %#04x\n",(unsigned int)(ledVal));
#endif
		gpioLedSet(ledVal);
		ledVal = ~ledVal & 0xff;
		mod_timer(t,get_jiffies_64() + timerVal);
}
///////////////////////////////////////////////////////////////////////////
struct file_operations kerneltimer_fops =
{
//    .owner    = THIS_MODULE,
    .open     = kerneltimer_open,     
    .read     = kerneltimer_read,     
    .write    = kerneltimer_write,    
	.unlocked_ioctl = kerneltimer_ioctl,
    .release  = kerneltimer_release,  
};
///////////////////////////////////////////////////////////////////////////
static int kerneltimer_init(void)
{
    int result;
    char * irqName[GPIOCNT] = {"irqKey0","irqKey1","irqKey2","irqKey3","irqKey4","irqKey5","irqKey6","irqKey7",};
    mutex_init(&keyMutex);

#if DEBUG
		printk("timerVal : %d , sec :%d\n", timerVal, timerVal / HZ);		
#endif

		result = gpioLedInit();
		if(result < 0)
				return result;

		result = gpioKeyInit();
		if(result < 0)
				return result;

		result = irqKeyInit();
		if(result < 0)
				return result;
		for(int i=0;i<GPIOCNT;i++)
		{
			result = request_irq(irqKey[i],keyIsr,IRQF_TRIGGER_RISING,irqName[i],NULL);
			if(result < 0 )
				return result;
		}
		kerneltimer_registertimer(timerVal);


		result = register_chrdev(KERNELTIMER_MAJOR, KERNELTIMER_NAME, &kerneltimer_fops);
		return 0;
}
///////////////////////////////////////////////////////////////////////////
static void kerneltimer_exit(void)
{
		unregister_chrdev( KERNELTIMER_MAJOR, KERNELTIMER_NAME);
		if(timer_pending(&timerLed))
				del_timer(&timerLed);
		gpioLedFree();
		irqKeyFree();
		gpioKeyFree();
		mutex_destroy(&keyMutex);

}
module_init(kerneltimer_init);
module_exit(kerneltimer_exit);
MODULE_AUTHOR("KCCI-AIOT KSH");
MODULE_DESCRIPTION("test module");
MODULE_LICENSE("Dual BSD/GPL");

