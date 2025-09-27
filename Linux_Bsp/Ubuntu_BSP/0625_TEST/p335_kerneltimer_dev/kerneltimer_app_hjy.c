#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME  "/dev/kerneltimer" // c 230 0

void print_led(unsigned char);
int main(int argc,char * argv[])
{
    int dev;
    char buff = 0;
	char oldBuff = 0;
    int ret;
	if(argc < 2)
	{
		printf("USAGE : %s [ledval] \n",argv[0]);
		return 1;
	}
//	buff = atoi(argv[1]);
	buff = (char)strtoul(argv[1],0,16);
	printf("buff : %#04x\n",buff);
	
    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
	if(dev<0)
	{
		perror("open()");
		return 2;
	}
    ret = write(dev,&buff,sizeof(buff));
	if(ret < 0)
	{
		perror("write()");
		return 3;
	}
	print_led(buff);
	
	buff = 0;
	do {
		read(dev,&buff,sizeof(buff));  // 인터럽트로 했던 방식으로 가져오자
		buff = 1 << buff-1;
		if((buff != 0) && (oldBuff != buff))
		{
			printf("key : %#04x\n",buff);
    		write(dev,&buff,sizeof(buff)); // 8개 중에 하나가 켜질 것이다. 
			print_led(buff);
			oldBuff = buff;
			if(buff == 0x80) //key:8
				break;
		}
	} while(1);


    close(dev);
    return 0;
}
void print_led(unsigned char led)
{
	int i;
	puts("1:2:3:4:5:6:7:8");
	for(i=0;i<=7;i++)
	{
		if(led & (0x01 << i))
			putchar('O');
		else
			putchar('X');
		if(i < 7 )
			putchar(':');
		else
			putchar('\n');
	}
	return;
}
