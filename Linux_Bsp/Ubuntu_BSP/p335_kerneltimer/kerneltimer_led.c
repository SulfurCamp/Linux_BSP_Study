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

//==================================LED/KEY 셋팅===========================
#define DEBUG 1
#define LED_OFF 0
#define LED_ON 1
#define GPIOCNT 8


static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};
static int gpioKeyInit(void);
static void gpioKeyGet(char *);
static void gpioKeyFree(void);
//=================================kerneltimer 셋팅=============================
static int timerVal = 100; //f=100HZ, T=1/100 = 10ms, 100*10ms = 1Sec
module_param(timerVal,int,0);
static int ledVal = 0;
module_param(ledVal,int,0);
struct timer_list timerLed;

static void kerneltimer_func(struct timer_list *t);
//=================================LED/KEY제어코드===================================
static int gpioLedInit(void)
{
	int i
    int ret = 0;
	char name[10];
	for(i = 0; i < GPIOCNT; i++) {
		sprintf(name, "led%d", i);
		ret = gpio_request(gpioLed[i], name);
		if(ret < 0) {
			printk("Failed to request GPIO %d\n", gpioLed[i]);
			return ret;
		}
		ret = gpio_direction_output(gpioLed[i], LED_OFF);
		if(ret < 0) {
			printk("Failed to set direction for GPIO %d\n", gpioLed[i]);
			return ret;
		}
	}
	return ret;
}

static void gpioLedSet(long val)
{
	int i;
	for(i = 0; i < GPIOCNT; i++) {
		gpio_set_value(gpioLed[i], (val >> i) & 0x01);
	}
}

static void gpioLedFree(void)
{
	int i;
	for(i = 0; i < GPIOCNT; i++) {
		gpio_free(gpioLed[i]);
	}
}

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
static void	gpioKeyGet(char *pKbuff)
{
	int i;
//	int ret;
//	int keyData=0;
	for(i=0;i<GPIOCNT;i++)
	{
//		ret=gpio_get_value(gpioKey[i]) << i;
//		keyData |= ret;
//		ret=gpio_get_value(gpioKey[i]);
//		keyData = keyData | ( ret << i );
		pKbuff[i]=gpio_get_value(gpioKey[i]);
	}
//	return keyData;
}
static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

//========================================================================
static void  kerneltimer_registertimer(unsigned long timeover)
{
    timerLed.expires = get_jiffies_64() + timeover; //10ms * 100 = 1sec
    timer_setup( &timerLed, kerneltimer_func, 0 );
    add_timer(&timerLed);
}

static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
    printk("ledVal : %#04x\n",(unsigned int)(ledVal));
#endif
    gpioLedSet(ledVal); // LED가 켜지는 기능을 추가할 것
    ledVal = ~ledVal & 0xff;
    mod_timer(t,get_jiffies_64() + timerVal);
}

static int  kerneltimer_init(void)
{
    int ret;
#if DEBUG
    printk("timerVal : %d, sec : %d \n",timerVal,timerVal/HZ);
#endif
    ret = gpioLedInit();
    if(ret < 0)
        return ret;
    kerneltimer_registertimer(timerVal);
    return 0;    
}

static void kerneltimer_exit(void)
{
    gpioLedFree();
    //gpioKeyFree();
    if(timer_pending(&timerLed))
    {
        del_timer(&timerLed);
    }
  
}

module_init(kerneltimer_init);
module_exit(kerneltimer_exit);

MODULE_AUTHOR("KCCI-AIOT KSH");
MODULE_DESCRIPTION("test module");
MODULE_LICENSE("Dual BSD/GPL");
