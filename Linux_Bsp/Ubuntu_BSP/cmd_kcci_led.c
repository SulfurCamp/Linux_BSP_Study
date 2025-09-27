#include <vsprintf.h>
#include <command.h>
#include <asm/io.h>
#include <linux/delay.h>

// BCM2711 GPIO 레지스터 주소
#define BCM2711_GPIO_GPFSEL0   0xFE200000
#define BCM2711_GPIO_GPFSEL1   0xFE200004
#define BCM2711_GPIO_GPFSEL2   0xFE200008
#define BCM2711_GPIO_GPSET0    0xFE20001C
#define BCM2711_GPIO_GPCLR0    0xFE200028
#define BCM2711_GPIO_GPLEV0    0xFE200034

// GPIO 기능 설정 값
#define GPIO6_9_SIG_OUTPUT     0x09240000
#define GPIO10_13_SIG_OUTPUT   0x00012249   // txd, rxd 관련

// LED 초기화: GPIO 핀을 출력으로 설정
void led_init(void)
{
    writel(GPIO6_9_SIG_OUTPUT, BCM2711_GPIO_GPFSEL0);
    writel(GPIO10_13_SIG_OUTPUT, BCM2711_GPIO_GPFSEL1);
}
void key_init(void)
{
    unsigned long val;

    // GPFSEL1: GPIO16~19 입력 설정
    val = readl(BCM2711_GPIO_GPFSEL1);
    val &= 0xC003FFFF; // clear
    val |= 0x09240000;
    writel(val, BCM2711_GPIO_GPFSEL1);

    // GPFSEL2: GPIO20~23 입력 설정
    val = readl(BCM2711_GPIO_GPFSEL2);
    val &= 0xFFFFF000; // clear
    val |= 0x00000249;
    writel(val, BCM2711_GPIO_GPFSEL2);
}
// LED 데이터 쓰기
void led_write(unsigned long led_data)
{
    writel(0x3fc0, BCM2711_GPIO_GPCLR0);      // 모든 LED OFF
    led_data = led_data << 6;                 // GPIO6 ~ GPIO13에 매핑
    writel(led_data, BCM2711_GPIO_GPSET0);    // 해당 비트 ON
}
void key_read(unsigned long *key_data)
{
    unsigned long val = readl(BCM2711_GPIO_GPLEV0);
    *key_data = (val >> 16) & 0xFF;
}
// U-Boot 명령 처리 함수
static int do_KCCI_LED(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long led_data;
    unsigned long key_data;

	if (argc != 2) {
        cmd_usage(cmdtp);
        return 1;
    }

    printf("*LED TEST START\n");
    led_init();
	key_init();
    led_data = simple_strtoul(argv[1], NULL, 16);
    led_write(led_data);
//====================================================	
	while(1)
    {
        int i;
		key_read(&key_data);

		for(i = 0; i < 8; i++){
			if(key_data & (1<<i)){

				led_write(key_data);
				printf("0:1:2:3:4:5:6:7\n");

				for(int k = 0; k < 8; k++){
					if(k == i)
						printf("O:");
					else
						printf("X:");
				}
				printf("\n\n");
				if(i == 7){
					printf("*LED TEST END(%s : %#04x)\n\n", argv[0], (unsigned int)led_data);
					return 0;
				}

				do {
   					key_read(&key_data);
				} while (key_data & (1 << i));
			}
		}
	}
//====================================================
}

// U-Boot 명령 등록
U_BOOT_CMD(
    led, 2, 0, do_KCCI_LED,
    "led - kcci LED Test.",
    "number - Input argument is only one. (led [0x00~0xff])\n"
);

