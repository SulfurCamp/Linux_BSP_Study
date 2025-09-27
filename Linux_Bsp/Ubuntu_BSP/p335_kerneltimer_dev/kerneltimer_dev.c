#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "ioctl_test.h"

///////////////////////////////////////////////////////////////////////////
#define DEBUG             1
#define GPIOCNT           8
#define LED_OFF           0
#define KERNELTIMER_NAME  "kerneltimer"
#define KERNELTIMER_MAJOR 230
///////////////////////////////////////////////////////////////////////////
static int gpioKey[GPIOCNT] = {528, 529, 530, 531, 532, 533, 534, 535};
static int gpioLed[GPIOCNT] = {518, 519, 520, 521, 522, 523, 524, 525};
static int irqKey[GPIOCNT];

static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);
static int gpioKeyInit(void);
// static int gpioKeyGet(void);
static void gpioKeyFree(void);
///////////////////////////////////////////////////////////////////////////
static int openFlag = 0;
static int timerVal = 100;
module_param(timerVal, int, 0);
static int ledVal = 0;
module_param(ledVal, int, 0);
static struct timer_list timerLed;
static void kerneltimer_func(struct timer_list *t);
static int keyNum = 0;

static DEFINE_MUTEX(keyMutex);
static DECLARE_WAIT_QUEUE_HEAD(readWaitQueue);

///////////////////////////////////////////////////////////////////////////
static irqreturn_t keyIsr(int irq, void *data)
{
    int i;
    for (i = 0; i < GPIOCNT; i++)
    {
        if (irq == irqKey[i])
        {
            if (mutex_trylock(&keyMutex) != 0)
            {
                keyNum = i + 1;
                //				if (keyNum == 8) {
                //    				del_timer(&timerLed);
                //    				gpioLedSet(0x00);
                //				}
                mutex_unlock(&keyMutex);
                break;
            }
        }
    }
#if DEBUG
    printk("keyIsr() irq : %d, keyNum : %d\n", irq, keyNum);
#endif
    wake_up_interruptible(&readWaitQueue);
    return IRQ_HANDLED;
}
///////////////////////////////////////////////////////////////////////////
static int gpioLedInit(void)
{
    int i;
    int ret = 0;
    char gpioName[10];
    for (i = 0; i < GPIOCNT; i++)
    {
        sprintf(gpioName, "led%d", i);
        ret = gpio_request(gpioLed[i], gpioName);
        if (ret < 0)
        {
            printk("Failed request gpio%d error\n", gpioLed[i]);
            return ret;
        }
    }
    for (i = 0; i < GPIOCNT; i++)
    {
        if (ret < 0)
        {
            printk("Failed direction_output gpio%d error\n", gpioLed[i]);
            return ret;
        }
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////
static void gpioLedSet(long val)
{
    int i;
    for (i = 0; i < GPIOCNT; i++)
    {
        gpio_set_value(gpioLed[i], ((val >> i) & 0x01));
    }
}
static void gpioLedFree(void)
{
    int i;
    for (i = 0; i < GPIOCNT; i++)
    {
        gpio_free(gpioLed[i]);
    }
}
///////////////////////////////////////////////////////////////////////////
static int gpioKeyInit(void)
{
    int i, ret = 0;
    char gpioName[10];
    for (i = 0; i < GPIOCNT; i++)
    {
        sprintf(gpioName, "key%d", i);
        ret = gpio_request(gpioKey[i], gpioName);
        if (ret < 0)
        {
            printk("Failed Request gpio%d error\n", gpioKey[i]);
            return ret;
        }
        ret = gpio_direction_input(gpioKey[i]);
        if (ret < 0)
        {
            printk("Failed direction_output gpio%d error\n", gpioKey[i]);
            return ret;
        }
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////
static int irqKeyInit(void)
{
    int i, ret = 0;
    for (i = 0; i < GPIOCNT; i++)
    {
        irqKey[i] = gpio_to_irq(gpioKey[i]);
        if (irqKey[i] < 0)
            return irqKey[i];
#if DEBUG
        printk("gpio_to_irq() gpio%d (irq%d)\n", gpioKey[i], irqKey[i]);
#endif
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////
static void irqKeyFree(void)
{
    int i;
    for (i = 0; i < GPIOCNT; i++)
    {
        free_irq(irqKey[i], NULL);
    }
}
///////////////////////////////////////////////////////////////////////////
static void gpioKeyFree(void)
{
    int i;
    for (i = 0; i < GPIOCNT; i++)
    {
        gpio_free(gpioKey[i]);
    }
}
///////////////////////////////////////////////////////////////////////////
static int kerneltimer_open(struct inode *inode, struct file *filp)
{
#if DEBUG
    int num0 = MAJOR(inode->i_rdev);
    int num1 = MINOR(inode->i_rdev);
    printk("call open -> major : %d\n", num0);
    printk("call open -> minor : %d\n", num1);
#endif
    if (openFlag)
        return -EBUSY;
    openFlag = 1;
    if (!try_module_get(THIS_MODULE))
        return -ENODEV;
    return 0;
}
///////////////////////////////////////////////////////////////////////////
// ioctl 기능
static void kerneltimer_start(void) { mod_timer(&timerLed, jiffies + timerVal); }

static void kerneltimer_stop(void) { del_timer(&timerLed); }
static long kerneltimer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
    case TIMER_START:
        kerneltimer_start();
        break;
    case TIMER_STOP:
        kerneltimer_stop();
        break;
    case TIMER_VALUE:
    {
        keyled_data kdata;
        if (copy_from_user(&kdata, (void __user *)arg, sizeof(kdata)))
            return -EFAULT;
        timerVal = kdata.timer_val;
#if DEBUG
        printk("TIMER_VALUE ioctl: timerVal = %d\n", timerVal);
#endif
        break;
    }
    default:
        return -EINVAL;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////
// 뮤텍스 사용 이유: keyNum이 전역변수이다. 근데 이게 인터럽트 함수에서도 쓰이고 read 함수에서도
// 쓰이는데 값이 동시에 접근이 되버리면 문제가 생길 수 있으므로 뮤텍스를 쓴다.
static ssize_t kerneltimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    char kbuf = 0;

    if (!(filp->f_flags & O_NONBLOCK))
    {
        // keyNum이 0이면 블록 상태 진입
        wait_event_interruptible(readWaitQueue, keyNum != 0);
    }

    if (mutex_trylock(&keyMutex) != 0)
    {
        if (keyNum != 0)
        {
            kbuf = (char)keyNum;
            keyNum = 0;
        }
        mutex_unlock(&keyMutex);
    }
    if (put_user(kbuf, buf))
        return -EFAULT;

    return sizeof(kbuf);
}
///////////////////////////////////////////////////////////////////////////
static ssize_t kerneltimer_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    char kbuf;
    get_user(kbuf, buf);
    ledVal = kbuf;
    return sizeof(kbuf);
}
///////////////////////////////////////////////////////////////////////////
static __poll_t kerneltimer_poll(struct file *filp, struct poll_table_struct *wait)
{
    __poll_t mask = 0;
    poll_wait(filp, &readWaitQueue, wait); //  poll이 wait queue 감시

    if (keyNum > 0)
        mask |= POLLIN; // 읽을 준비됨

    return mask;
}
///////////////////////////////////////////////////////////////////////////
static int kerneltimer_release(struct inode *inode, struct file *filp)
{
#if DEBUG
    printk("call release\n");
#endif
    del_timer(&timerLed); // 타이머 정지 추가
    module_put(THIS_MODULE);
    openFlag = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////
static void kerneltimer_registertimer(unsigned long timeover)
{
    timerLed.expires = get_jiffies_64() + timeover;
    timer_setup(&timerLed, kerneltimer_func, 0);
    add_timer(&timerLed);
}
///////////////////////////////////////////////////////////////////////////
static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
    printk("ledVal : %#04x\n", (unsigned int)(ledVal));
#endif
    int i;
    for (i = 0; i < GPIOCNT; i++)
    {
        gpio_set_value(gpioLed[i], (ledVal >> i) & 0x01); // --------------------
    }
    ledVal = ~ledVal & 0xff;
    mod_timer(t, get_jiffies_64() + timerVal);
}
///////////////////////////////////////////////////////////////////////////
static struct file_operations kerneltimer_fops = {
    //.owner = THIS_MODULE,
    .open = kerneltimer_open,
    .read = kerneltimer_read,
    .write = kerneltimer_write,
    .release = kerneltimer_release,
    .unlocked_ioctl = kerneltimer_ioctl,
    .poll = kerneltimer_poll,
};
///////////////////////////////////////////////////////////////////////////
static int __init kerneltimer_init(void)
{
    int result;
    char *irqName[GPIOCNT] = {"irqKey0", "irqKey1", "irqKey2", "irqKey3",
                              "irqKey4", "irqKey5", "irqKey6", "irqKey7"};

    printk("call ledkey_init\n");
    mutex_init(&keyMutex);

    result = gpioLedInit();
    if (result < 0)
        return result;

    result = gpioKeyInit();
    if (result < 0)
        return result;

    result = irqKeyInit();
    if (result < 0)
        return result;

    for (int i = 0; i < GPIOCNT; i++)
    {
        result = request_irq(irqKey[i], keyIsr, IRQF_TRIGGER_RISING, irqName[i], NULL);
        if (result < 0)
            return result;
    }

    kerneltimer_registertimer(timerVal);

    result = register_chrdev(KERNELTIMER_MAJOR, KERNELTIMER_NAME, &kerneltimer_fops);
    if (result < 0)
        return result;

    return 0;
}
///////////////////////////////////////////////////////////////////////////
static void __exit kerneltimer_exit(void)
{
    printk("call ledkey_exit\n");
    unregister_chrdev(KERNELTIMER_MAJOR, KERNELTIMER_NAME);
    if (timer_pending(&timerLed))
        del_timer(&timerLed);
    gpioLedFree();
    irqKeyFree();
    gpioKeyFree();
    mutex_destroy(&keyMutex);
}
///////////////////////////////////////////////////////////////////////////
module_init(kerneltimer_init);
module_exit(kerneltimer_exit);

MODULE_AUTHOR("KCCI");
MODULE_DESCRIPTION("led key timer driver");
MODULE_LICENSE("Dual BSD/GPL");
