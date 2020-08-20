#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

static int fd = 0;

/* SIGIO信号处理函数 */
static void sigio_signal_func(int signum)
{
    int err = 0;
    unsigned int keyvalue = 0;

    err = read(fd, &keyvalue, sizeof(keyvalue));
    if (err < 0){

    }else{
        printf("sigio signal! key value = %d\r\n", keyvalue);
    }
}


int main(int argc, char* argv[])
{
    int ret = 0;
    char *filename;
    int flag = 0;

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

    /* 信号安装 */
    signal(SIGIO, sigio_signal_func);

    fcntl(fd, F_SETOWN, getpid());      // 将当前进程PID告诉内核
    flag = fcntl(fd, F_GETFD);          // 获取当前进程状态
    fcntl(fd, F_SETFL, flag|FASYNC);    // 设置进程启用异步通知功能

    while (1){
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
