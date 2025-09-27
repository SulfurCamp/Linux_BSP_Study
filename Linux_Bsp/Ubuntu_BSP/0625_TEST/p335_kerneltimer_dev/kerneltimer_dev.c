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
#include <linux/poll.h>
#include <linux/wait.h>

#include "ioctl_test.h"

///////////////////////////////////////////////////////////////////////////
#define DEBUG 1
#define GPIOCNT 8
#define LED_OFF 0
#define KERNELTIMER_NAME 	"kerneltimer"
#define	KERNELTIMER_MAJOR 	230
///////////////////////////////////////////////////////////////////////////
static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};
static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int irqKey[GPIOCNT];

static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);
static int gpioKeyInit(void);
//static int gpioKeyGet(void);
static void gpioKeyFree(void);
///////////////////////////////////////////////////////////////////////////
static int openFlag = 0;
static int timerVal = 100;
module_param(timerVal, int , 0);
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
    for(i = 0; i < GPIOCNT; i++) {
        if(irq == irqKey[i]) {
            if(mutex_trylock(&keyMutex) != 0) {
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
    for(i = 0; i < GPIOCNT; i++) {
        gpio_free(gpioLed[i]);
    }
}
///////////////////////////////////////////////////////////////////////////
static int gpioKeyInit(void)
{
    int i, ret = 0;
    char gpioName[10];
    for(i = 0; i < GPIOCNT; i++) {
        sprintf(gpioName, "key%d", i);
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
    int i, ret = 0;
    for(i = 0; i < GPIOCNT; i++) {
        irqKey[i] = gpio_to_irq(gpioKey[i]);
        if(irqKey[i] < 0) return irqKey[i];
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
    for(i = 0; i < GPIOCNT; i++) {
        free_irq(irqKey[i], NULL);
    }
}
///////////////////////////////////////////////////////////////////////////
static void gpioKeyFree(void)
{
    int i;
    for(i = 0; i < GPIOCNT; i++) {
        gpio_free(gpioKey[i]);
    }
}
///////////////////////////////////////////////////////////////////////////
//2. 디바이스를 여는 부분입니다.
//중복 방지를 위한 openFlag
//모듈 참조 카운트 증가(try_module_get())
static int kerneltimer_open(struct inode *inode, struct file *filp)
{
#if DEBUG
    int num0 = MAJOR(inode->i_rdev);
	int num1 = MINOR(inode->i_rdev);
 	printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );
#endif
    if(openFlag)
        return -EBUSY;
    openFlag = 1;
    if(!try_module_get(THIS_MODULE))
        return -ENODEV;
    return 0;
}
///////////////////////////////////////////////////////////////////////////
//ioctl 기능
//6. TIMER_START 타이머 시작, TIMER_STOP 타이머 정지 TIMER_VALUE 주기 변경
//TIMER_START와 TIMER_STOP은 함수를 정의해서 mod_timer() del_timer() 함수를 호출하게끔함
//TIMER_VALUE 주기 변경 timerVal 업데이트
static void kerneltimer_start(void)
{
    mod_timer(&timerLed, jiffies + timerVal);
}

static void kerneltimer_stop(void)
{
    del_timer(&timerLed);
}
static long kerneltimer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
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
// 뮤텍스 사용 이유: keyNum이 전역변수이다. 근데 이게 인터럽트 함수에서도 쓰이고 read 함수에서도 쓰이는데 값이 동시에 접근이 되버리면 문제가 생길 수 있으므로 뮤텍스를 쓴다.
// 4. 사용자 read() 호출 Blocking or Poll 기반
// keyNum == 0이면 waitqueue에 대기 Blocking I/O
// keyIsr()에서 키눌림이 감지되면 wake up 
// keyNum을 사용자에게 전달하고 다시 0으로 초기화
static ssize_t kerneltimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    char kbuf = 0;

    if (!(filp->f_flags & O_NONBLOCK)) {
        // keyNum이 0이면 블록 상태 진입
        wait_event_interruptible(readWaitQueue, keyNum != 0);
    }

    if(mutex_trylock(&keyMutex) != 0) { 
        if(keyNum != 0) {
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
//3. 사용자로부터 LED값을 받아서 ledVal에 저장
// 실제 LED 제어는 타이머 핸들러에서 수행되게끔 작성
static ssize_t kerneltimer_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    char kbuf;
    get_user(kbuf, buf);
    ledVal = kbuf;
    return sizeof(kbuf);
}
///////////////////////////////////////////////////////////////////////////
//5. poll() 함수 호출
// 키가 눌렸는지 keyNum > 0 여부를 확인
// 눌림이 확인되면 POLLIN 반환 -> 사용자 앱에서 read 가능상태를 전송
static __poll_t kerneltimer_poll(struct file *filp, struct poll_table_struct *wait)
{
    __poll_t mask = 0;
    poll_wait(filp, &readWaitQueue, wait);       //  poll이 wait queue 감시

    if (keyNum > 0)
        mask |= POLLIN;  // 읽을 준비됨

    return mask;
}
///////////////////////////////////////////////////////////////////////////
//8. 호출 앱종료 또는 close
// 타이머 중지, 모듈 참조 카운트 감소

static int kerneltimer_release(struct inode *inode, struct file *filp)
{
#if DEBUG
    printk("call release\n");
#endif
    del_timer(&timerLed);            // 타이머 정지 추가
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
//7. 타이머 동작
// ledVal을 GPIO로 출력
// ledVal을 반전시켜 다음 깜빠김 준비
// mod_timer()로 주기 재등록을 반복
static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
    printk("ledVal : %#04x\n", (unsigned int)(ledVal));
#endif
    int i;
    for(i = 0; i < GPIOCNT; i++) {
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
//1. insmod 하는 부분입니다. LED, KEY, GPIO 초기화
//IRQ 핸들러 등록
// timer_setup() 만 수행 타이머는 실행되지 않아야 함
// register_chardev()로 디바이스 드라이버 등록
// LED는 아직 깜빡이지 않는다. -> 사용자 요청 시에만 시작
static int __init kerneltimer_init(void)
{
    int result;
    char *irqName[GPIOCNT] = {"irqKey0","irqKey1","irqKey2","irqKey3","irqKey4","irqKey5","irqKey6","irqKey7"};

    printk("call ledkey_init\n");
    mutex_init(&keyMutex);

    result = gpioLedInit();
    if(result < 0) return result;

    result = gpioKeyInit();
    if(result < 0) return result;

    result = irqKeyInit();
    if(result < 0) return result;

    for(int i = 0; i < GPIOCNT; i++) {
        result = request_irq(irqKey[i], keyIsr, IRQF_TRIGGER_RISING, irqName[i], NULL);
        if(result < 0) return result;
    }

    timer_setup(&timerLed, kerneltimer_func, 0);

    result = register_chrdev(KERNELTIMER_MAJOR, KERNELTIMER_NAME, &kerneltimer_fops);
    if(result < 0) return result;

    return 0;
}
///////////////////////////////////////////////////////////////////////////
//9. 드라이버 제거
// 타이머 정리 GPIO/IRQ 자원 반납
// mutex 제거 드라이버 등록 해제

static void __exit kerneltimer_exit(void)
{
    printk("call ledkey_exit\n");
    unregister_chrdev( KERNELTIMER_MAJOR, KERNELTIMER_NAME);
    if(timer_pending(&timerLed)) del_timer(&timerLed);
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
