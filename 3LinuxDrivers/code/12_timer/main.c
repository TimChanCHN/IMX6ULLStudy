#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>

#define CLOSE_CMD       (_IO(0XEF, 0X01))
#define OPEN_CMD        (_IO(0XEF, 0X02))
#define SETPERIOD_CMD   (_IO(0XEF, 0X03))

int main(int argc, char* argv[])
{
    int fd = 0;
    int ret = 0;
    char *filename;
    unsigned int cmd;
    unsigned int arg;
    static char str[100];

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
        printf("Input CMD:");
        ret = scanf("%d", &cmd);
        if (ret != 1){
            //gets(str);
        }

        if (cmd == 1)
            cmd = CLOSE_CMD;
        else if (cmd == 2)
            cmd = OPEN_CMD;
        else if (cmd == 3){
            cmd = SETPERIOD_CMD;
            printf("Input Timer Period:");
            ret = scanf("%d", &arg);
            if (ret != 1){
                //gets(str);
            }
        }
        ioctl(fd, cmd, arg);
    }

    ret = close(fd);
    if (ret < 0)
    {
        printf("close file fail\r\n");
        return -1;
    }

    return 0;
}
