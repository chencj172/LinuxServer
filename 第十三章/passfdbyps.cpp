// 功能：在进程间传递文件描述符
// 2023年02月22日 20:11:19
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// 返回对齐后的长度
static const  int CONTROL_LEN = CMSG_LEN(sizeof(int));
// 发送文件描述符，fd参数是用来传递信息的UNIX域socket
// fd_to_send参数是待发送的文件描述符
void send_fd(int fd, int fd_to_send)
{
    // iovec 定义了一个向量元素，这个结构用作一个多元素数组
    struct iovec iov[1];
    struct msghdr msg;  // 用于接收和发送数据
    char buf[0];

    // iov_base指向一个缓冲区，这个缓冲区是存放的是readv所接收的数据或是writev将要发送的数据。
    iov[0].iov_base = buf;
    iov[0].iov_len = 1; // 接收的最大长度以及实际写入长度
    msg.msg_name = NULL; // 表示消息的目标地址，通常是一个指向 struct sockaddr 结构体的指针
    msg.msg_iov = iov;  // 数据缓冲区
    msg.msg_iovlen = 1; // 数据缓冲区中元素个数

    // 表示控制信息头部的结构体，通常为msghdr中的msg_control字段数据
    // 通常用于发送或接收一些与协议相关的控制信息，例如 IP 的 TTL 值、TCP 的选项等
    cmsghdr cm;
    // 表示辅助数据的长度，包括 struct cmsghdr 结构体的长度和后面跟随的辅助数据的长度。
    cm.cmsg_len = CONTROL_LEN;
    // 表示辅助数据所属协议的层级
    cm.cmsg_level = SOL_SOCKET;
    // 表示辅助数据的类型
    cm.cmsg_type = SCM_RIGHTS; // SCM_RIGHTS表面上传递的是文件描述符，但实际上并不是简单地传递描述符的数字，而是传递描述符背后的 file 文件
    // CMSG_DATA()返回一个指向cmsghdr的数据部分的指针
    *(int *)CMSG_DATA(&cm) = fd_to_send;
    msg.msg_control = &cm; // 设置辅助数据
    msg.msg_controllen = CONTROL_LEN;
    
    sendmsg(fd, &msg, 0);
}

// 接收目标文件描述符
int recv_fd(int fd)
{
    struct iovec iov[1];
    struct msghdr msg;  // 用于接收和发送数据
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1; // 接收的最大长度以及实际写入长度
    msg.msg_name = NULL; // 表示消息的目标地址，通常是一个指向 struct sockaddr 结构体的指针
    msg.msg_iov = iov;  // 数据缓冲区
    msg.msg_iovlen = 1; // 数据缓冲区中元素个数

    // 表示控制信息头部的结构体，通常为msghdr中的msg_control字段数据
    // 通常用于发送或接收一些与协议相关的控制信息，例如 IP 的 TTL 值、TCP 的选项等
    cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg(fd, &msg, 0);

    int fd_to_read = *(int *)CMSG_DATA(&cm);
    return fd_to_read;
}

int main()
{
    int pipefd[2];
    int fd_to_pass = 0;
    // 创建父子进程间的管道，文件描述符pipefd[0]和pipefd[1]都是UNIX域socket
    // 传递文件描述符只能用Unix域套接字
    int ret = socketpair(PF_UNIX, SOCK_DGRAM, 0, pipefd);
    // int ret = pipe(pipefd);
    assert(ret != -1);

    pid_t pid = fork();
    assert(pid >= 0);

    if(pid == 0) // child
    {
        close(pipefd[0]);
        fd_to_pass = open("test.txt", O_RDWR, 0666);
        std::cout << fd_to_pass << std::endl;
        // 子进程通过管道将文件描述符发送到父进程
        // 若test.txt 打开失败，则子进程将标准输入文件描述符发送到父进程
        send_fd(pipefd[1], (fd_to_pass) > 0 ? fd_to_pass : 0); //!!!
        close(fd_to_pass);
        exit(0);
    }
    // parent
    close(pipefd[1]);
    fd_to_pass = recv_fd(pipefd[0]); // 父进程从管道接收目标文件描述符
    char buf[1024];
    memset(buf, '\0', 1024);
    read(fd_to_pass, buf, 1024);
    printf("I got fd %d and data %s\n", fd_to_pass, buf);
    close(fd_to_pass);
}
