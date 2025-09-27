#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#define GPIOCNT 8
#define LED_OFF 0
#define LED_ON 1
//int gpioLed[GPIOCNT] = {6,7,8,9,10,11,12,13};
unsigned int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
int gpioLedInit(void);
void gpioLedSet(long val);
void gpioLedFree(void);
unsigned int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};
int gpioKeyInit(void);
long gpioKeyGet(void);
void gpioKeyFree(void);

int key_state = 0;

// LED 초기화 및 점등
int gpioLedInit(void)
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
void gpioLedSet(long val)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_set_value(gpioLed[i],((val >> i) & 0x01));
	}			
}
void gpioLedFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}

int gpioKeyInit(void)
{
	int i;
	int ret=0 ;
	char gpioName[10];
	for(i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"key%d",i);
		ret=gpio_request(gpioKey[i],gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n",gpioKey[i]);
			return ret;
		}
	}
	for(i=0;i<GPIOCNT;i++)
	{
		ret = gpio_direction_input(gpioKey[i]);	
		if(ret < 0)
		{
			printk("Failed direction_output gpio%d error\n",gpioKey[i]);
			return ret;
		}
	}
	return ret;
}
long gpioKeyGet(void)
{
	int i;
	int key=0;
	for(i=0;i<GPIOCNT;i++)
	{
		key |= gpio_get_value(gpioKey[i]) << i;
	}			
	return key;
}
void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

//=======================================================================================
int led_init_function(void)
{
	int ret=0;

	printk(KERN_INFO "[INIT] Module inserted: checking key state and lighting LED accordingly.\n");

	ret = gpioLedInit();
	if (ret < 0) return ret;

	ret = gpioKeyInit();
	if (ret < 0) {
		gpioLedFree();
		return ret;
	}

	key_state = gpioKeyGet(); // 현재 키 상태 저장
	gpioLedSet(key_state);    // 눌린 키에 해당하는 LED만 켜기
	gpioKeyFree();

	return 0;
}

static void led_exit_function(void)
{
	long current_key_state;

	printk(KERN_INFO "[EXIT] Module removed: checking key state to decide LED status.\n");

	if (gpioKeyInit() < 0) {
		gpioLedSet(0x00);
		gpioLedFree();
		return;
	}

	current_key_state = gpioKeyGet();
	gpioKeyFree();

	if (current_key_state == 0) {
		// 아무 키도 안 눌린 경우 LED 모두 소등
		gpioLedSet(0x00);
		printk(KERN_INFO "No keys pressed: turningpw off all LEDs.\n");
	} else {
		// 눌린 키에 해당하는 LED 상태 유지
		gpioLedSet(current_key_state);
		printk(KERN_INFO "Some keys still pressed: keeping corresponding LEDs ON.\n");
	}

	// 마지막에 LED만 해제 (GPIO는 해제하되 상태는 유지됨)
	gpioLedFree();
}


module_init(led_init_function);
module_exit(led_exit_function);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("GPIO LED Control Module - insmod: ON / rmmod: OFF");