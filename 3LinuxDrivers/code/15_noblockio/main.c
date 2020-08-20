#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char* argv[])
{
    int fd = 0;
    int ret = 0;
    char *filename;
    struct pollfd fds;
    fd_set readfds;
    struct timeval timeout;
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

#if 0
    fds.fd = fd;
    fds.events = POLLIN;

    while(1){
        ret = poll(&fds, 1, 500);
        if (ret){
            ret = read(fd, &data, sizeof(data));
            if (ret < 0){
                printf("read poll fail\r\n");
            }
            else{
                if (data)
                    printf("read data : %d\r\n", data);
            }
        } else if (ret == 0){
            printk("read data over time.\r\n");
        }
    }
#else
    while(1){
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        ret = select(fd+1, &readfds, NULL, NULL, &timeout);
        switch(ret){
            case 0:
                // printf("read over time!\r\n");
                break;
            
            case -1:
                // printf("read error!\r\n");
                break;
            default:
                if (FD_ISSET(fd, &readfds)){
                    ret = read(fd, &data, sizeof(data));
                    if (ret < 0)
                    {
                        printf("read error!!!\r\n");
                    }
                    else{
                        if (data)
                        {
                            printf("key value = %d \r\n", data);
                        }
                    }
                }
        }
    }
#endif

    ret = close(fd);
    if (ret < 0)
    {
        printf("close file fail\r\n");
        return -1;
    }

    return 0;
}
