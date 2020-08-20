#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    int fd = 0;
    int ret = 0;
    char *filename;
    char readbuf[100], writebuf[100];
    static char usrdata[] = "user data";

    if (argc != 3)
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

    printf("running...\r\n");
    printf("cmd = %s\r\n", argv[2]);
    if (atoi(argv[2]) == 1)
    {
        printf("222\r\n");
        ret = read(fd, readbuf, 50);
        if (ret < 0)
        {
            printf("read file %s fail!\r\n", filename);
        }
        else
        {
            printf("read data:%s\r\n", readbuf);
        } 
    }
    else if (atoi(argv[2]) == 2)
    {
        printf("111\r\n");
        ret = write(fd, usrdata, sizeof(usrdata));
        if (ret < 0)
        {
            printf("write file error!\r\n");
        }
        else
        {
            printf("write data success!\r\n");
        }
    }

    // ret = close(fd);
    // if (ret < 0)
    // {
    //     printf("close file fail\r\n");
    //     return -1;
    // }

    return 0;
}
