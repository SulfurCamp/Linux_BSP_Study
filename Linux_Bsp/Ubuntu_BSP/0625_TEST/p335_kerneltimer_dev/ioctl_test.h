#ifndef __IOCTL_H__
#define __IOCTL_H__

#define IOCTLTEST_MAGIC '6' 
typedef struct
{
	unsigned long size;
	unsigned char buff[128];
} __attribute__((packed)) ioctl_test_info;

//커널 타이머용 구조체
typedef struct {
    unsigned long timer_val;
} __attribute__((packed)) keyled_data;

char buff = 0;
#define IOCTLTEST_KEYLEDINIT 	_IO(IOCTLTEST_MAGIC, 0) 
#define IOCTLTEST_KEYLEDFREE 	_IO(IOCTLTEST_MAGIC, 1)
#define IOCTLTEST_LEDOFF		_IO(IOCTLTEST_MAGIC, 2)
#define IOCTLTEST_LEDON			_IO(IOCTLTEST_MAGIC, 3)
#define IOCTLTEST_GETSTATE		_IO(IOCTLTEST_MAGIC, 4)
#define IOCTLTEST_READ			_IOR(IOCTLTEST_MAGIC, 5,ioctl_test_info)
#define IOCTLTEST_WRITE			_IOW(IOCTLTEST_MAGIC, 6,ioctl_test_info)
#define IOCTLTEST_WRITE_READ	_IOWR(IOCTLTEST_MAGIC, 7,ioctl_test_info)
#define IOCTLTEST_KEYINIT 		_IO(IOCTLTEST_MAGIC, 8) 
#define IOCTLTEST_LEDINIT 		_IO(IOCTLTEST_MAGIC, 9) 
#define IOCTLTEST_LEDKEY		_IOWR(IOCTLTEST_MAGIC,10,char)

// 커널 타이머 명령 매크로
#define TIMER_START 			_IO('T', 1)
#define TIMER_STOP  			_IO('T', 2)
#define TIMER_VALUE 			_IOW('T', 3, keyled_data)

#define IOCTLTEST_MAXNR			11
#endif
