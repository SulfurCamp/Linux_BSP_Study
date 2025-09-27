#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#define GPIOCNT 8
#define LED_OFF 0
#define LED_ON 1

#define CALL_DEV_NAME    "calldev"
#define CALL_DEV_MAJOR   230

typedef struct {
	int num;
	//char name[10];
	char *pName;
}Student;

static unsigned int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);
static unsigned int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};
static int gpioKeyInit(void);
static long gpioKeyGet(void);
static void gpioKeyFree(void);

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

static int gpioKeyInit(void)
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
static long gpioKeyGet(void)
{
	int i;
	int key=0;
	for(i=0;i<GPIOCNT;i++)
	{
		key |= gpio_get_value(gpioKey[i]) << i;
	}			
	return key;
}
static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

/*========================================================================================*/
static int ledkey_open(struct inode *inode, struct file *filp)
{
	Student *pStudent; 
    int num0 = MINOR(inode->i_rdev);
    int num1 = MAJOR(inode->i_rdev);
    printk("call open -> minor : %d\n", num0);
    printk("call open -> major : %d\n", num1);
    try_module_get(THIS_MODULE);

	pStudent =(Student *)kmalloc(sizeof(Student), GFP_KERNEL);
	if(pStudent == NULL)
		return -ENOMEM;
	pStudent -> num=1234;
	pStudent -> pName="hong gil dong";

	filp->private_data=pStudent;
	return 0;

}
static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    int key_val;

	Student *pStudent = (Student *)filp->private_data;

    printk("call read -> buf : %08X, count : %08X\n", (unsigned int)buf, count);
    printk("num : %d, name : %s\n", pStudent->num, pStudent->pName);
	key_val = gpioKeyGet();
    return key_val;
}

static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *_pos)
{
    printk("call write -> buf : %08X, count : %08X\n", (unsigned int)buf, count);
    gpioLedSet((unsigned int)buf);
    return 0x43;
}

static long ledkey_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk("call ioctl -> cmd : %08X, arg : %08X\n", cmd, (unsigned int)arg);
    return 0x53;
}


static int ledkey_release(struct inode *inode, struct file *filp)
{
    printk("call release\n");
    kfree(filp->private_data);
	return 0;
}

struct file_operations ledkey_fops = {
    .owner    = THIS_MODULE,
    .open     = ledkey_open,
    .read     = ledkey_read,
    .write    = ledkey_write,
    .unlocked_ioctl = ledkey_ioctl,
    .release  = ledkey_release,
};

static int ledkey_init(void)
{
    int result;
    printk("call ledkey_init\n");
    result = register_chrdev(CALL_DEV_MAJOR, CALL_DEV_NAME, &ledkey_fops);
    if(result < 0) return result;

    int ret = 0;
    ret = gpioLedInit();    //gpio_request(), gpiot_direction_output();
	if(ret < 0)
		return ret;

	ret = gpioKeyInit();
    if(ret < 0)
        return ret;

    //ret = gpioKeyGet();     //val : 0x00 ~ 0xff
    //gpioLedSet(onevalue);        //gpio_set_value();

    return 0;
}

static void ledkey_exit(void)
{
    printk("call ledkey_exit\n");
    unregister_chrdev(CALL_DEV_MAJOR, CALL_DEV_NAME);

    gpioLedFree();          //gpio_free();
    gpioKeyFree();          //gpio_free();
}

module_init(ledkey_init);
module_exit(ledkey_exit);
MODULE_LICENSE("Dual BSD/GPL");
