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
    unsigned char databuf[1];
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

    databuf[0] = atoi(argv[2]);
    ret = write(fd, databuf, sizeof(databuf));
    if (ret < 0)
    {
        printf("BEEP Control fail.\r\n");
        close(fd);
        return -1;
    }

    ret = close(fd);
    if (ret < 0)
    {
        printf("close file fail\r\n");
        return -1;
    }

    return 0;
}
