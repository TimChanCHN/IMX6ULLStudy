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
    unsigned char data;

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
        ret = read(fd, &data, sizeof(data));
        if (ret < 0)
        {
            // printf("read fail\r\n");
        }else{
            if (data){
                printf("key value = %d\r\n", data);
            }
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
