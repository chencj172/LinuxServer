#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>

static int a = 1;
int *b = new int(2);
int main(int argc, const char* argv[])
{
    pid_t pid = fork();
    // 创建子进程后,子进程共享的是父进程的虚拟地址空间,虚拟地址的值是一样的,但是回映射不同的物理地质
    if(pid == 0)
    {
        sleep(2);
        std::cout << "a = " << a  << ", b = " << *b << std::endl;
        std::cout << "child a : " << &a << std::endl;
        std::cout << "child b : " << b << std::endl;
        exit(0);
    }
    else
    {
        a = 10;
        *b = 10;
        std::cout << "father a : " << &a << std::endl;
        std::cout << "father b : " << b << std::endl;
    }

    sleep(5);
    int stat;
    waitpid(pid, &stat, WNOHANG);
    delete b;
    return 0;
}