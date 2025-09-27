#include <stdio.h>
#include <unistd.h>
#include <asm-generic/unistd.h>
#include "/home/ubuntu/pi_bsp/kernel/linux/include/uapi/asm-generic/unistd.h"
#pragma GCC diagnostic ignored "-Wunused-result"

int main(int argc, char* argv[]) {
    long key_data = 0;
    long key_data_old = 0;
    long val;

    printf("input value (0x00 ~ 0xff) = ");
    scanf("%ld", &val);

    printf("*LED TEST START\n");

    do {
        key_data = syscall(__NR_mysyscall, val);

        if (key_data < 0) {
            perror("syscall");
            return 1;
        }

        if (key_data != key_data_old) {
            if (key_data) {
                val = key_data;

                syscall(__NR_mysyscall, val);

                printf("0:1:2:3:4:5:6:7\n");
                for (int i = 0; i < 8; i++) {
                    printf("%c", (val & (1 << i)) ? 'O' : 'X');
                    printf((i != 7) ? ":" : "\n");
                }
                printf("\n");

                if (key_data == 0x80) {
                    break;
                }
            }
            key_data_old = key_data;
        }
    } while (1);

    syscall(__NR_mysyscall, -1);  // 리소스 정리용
    printf("*LED TEST END(%s : %#04lx)\n", argv[0], key_data);

    return 0;
}
