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
    unsigned short databuf[3];
    unsigned short ir, als, ps;

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
        ret = read(fd, databuf, sizeof(databuf));
        if (ret == 0){
            ir = databuf[0];
            als = databuf[1];
            ps = databuf[2];
            printf("ir = %d, als = %d, ps = %d\r\n", ir, als, ps);
        }
        sleep(2);
    }

    ret = close(fd);
    if (ret < 0)
    {
        printf("close file fail\r\n");
        return -1;
    }

    return 0;
}