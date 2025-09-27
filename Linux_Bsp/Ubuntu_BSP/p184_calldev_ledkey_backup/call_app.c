#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME "/dev/ledkey"

int main(void)
{
    int dev;
    char buff;
    int ret;

    if(argc < 2)
    {
        printf("USAGE : %s ledVal[0x00~0xff]\n",argv[0]);
        return 1;
    }
    if(access(DEVICE_FILENAME, F_OK) != 0) //파일 없을 시
    {
        int ret = mknod(DEVICE_FILENAME, S_IRWXU | S_IRWXG | S_IFCHR, (230 << 8) | 0);
        
        if(ret<0)
            perror("mknod()");
    }
    printf("1) device file open\n");

    dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);
    if(dev >= 0)
    {
        perror("open()");
        return 2;
    }
    //while(1);
    ret = read(dev, &buff, sizeof(buff));
    if(ret < 0)
    {
        perror("read()");
        return 3; 
    }


    printf("ret = %08X, key = %#04x\n", ret, (unsigned int)buff);
        
    buff = 0x55;
    ret = write(dev, &buff, sizeof(buff));
    printf("ret = %08X\n", ret);

    ret = close(dev);
    return 0;
    
}
