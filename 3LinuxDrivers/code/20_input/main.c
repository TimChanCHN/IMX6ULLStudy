#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <linux/input.h>

static struct input_event inputevent;

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
        ret = read(fd, &inputevent, sizeof(inputevent));
        if (ret < 0)
        {
            // printf("read fail\r\n");
        }else{
            switch (inputevent.type){
                case EV_KEY:
                    if (inputevent.code < BTN_MISC){
                        printf("key %d %s\r\n", inputevent.code, 
                                inputevent.value?"press":"release");
                    }else{
                        printf("button %d %s \r\n", inputevent.code,
                                inputevent.value?"press":"release");
                    }
                    break;
                default:
                    break;
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
