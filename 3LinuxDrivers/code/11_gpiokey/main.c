#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define KEY0VALUE    0XF0
#define INVAKEY      0X00

int main(int argc, char* argv[])
{
    int fd = 0;
    int ret = 0;
    char *filename;
    char readbuf[100], writebuf[100];
    unsigned char databuf[1];
    unsigned char cnt = 0;
    unsigned char keyvalue;
    static char usrdata[] = "user data";

    if (argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0)
    {
        printf("open file %s fail!\r\n", filename);
        return -1;
    }

    while (1){
        read(fd, &keyvalue, sizeof(keyvalue));
        if (keyvalue == KEY0VALUE)
        {
            printf("KEY0 Pressed, value = %#X\r\n", keyvalue);
        }
    }

    ret = close(fd);
    if (ret < 0)
    {
        printf("close file fail\r\n");
        return -1;
    }

    return 0;
}
