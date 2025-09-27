#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#define DEVICE_FILENAME  "/dev/ledkey"

int main(int argc, char *argv[])
{
    int dev;
    int ret;
    int led_val;
    char buff;
    char key_val = 0;
    char key_old = 0;
    
	if(argv < 2)
    {
        printf("USAGE : %s ledVal[0x00~0xff]\n",argv[0]);
        return 1;
    }
	
    led_val = strtoul(argv[1], NULL, 16);
    //buff = (char)led_val;

    if(access(DEVICE_FILENAME,F_OK) != 0)   //파일 없을시
    {
        int ret = mknod(DEVICE_FILENAME, S_IRWXU | S_IRWXG | S_IFCHR, (230 << 8) | 0);
        if(ret < 0)
        {
            perror("mknod()");
            return 2;
        }
    }

    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY);

    if(dev < 0)
    {
		perror("open");
		return 3;
	}

    buff = (char)led_val;

    write(dev, &buff, sizeof(buff));

    printf("*LED-KEY TEST START\n");

    while(1){
        ret = read(dev,&buff, sizeof(buff));
        if(ret < 0)
        {
    		perror("read()");
		    break;
        }
        key_val = buff;
        if(key_val!=key_old)
        {
            key_old = key_val;
            if(key_val)
            {
                buff = key_val;
                write(dev,&buff,sizeof(buff));

                printf("0:1:2:3:4:5:6:7\n");
                for(int i=0;i<8;i++)
                {
                    printf("%c",( buff & (1<<i) ) ? 'O' : 'X' );
                    if(i<7) printf(":");
                }
                printf("\n\n");

                if(key_val == 0x80)
                    break;
            }
        }
    }	
    ret = close(dev);
    printf("*LED-KEY TEST END\n");
    return 0;
    // printf( "ret = %08X, key = %#04x\n",ret, buff );

	// buff = 0x55;
	// ret = write(dev,&buff,sizeof(buff));
	// printf( "ret = %08X\n", ret );

	// ret = close(dev);
    // return 0;
}

