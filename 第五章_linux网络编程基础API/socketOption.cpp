#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, const char* argv[])
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    int reuse = 0;
    socklen_t len;
    // 得到socket的相关配置并且值存到reuse中
    getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, &len);
    std::cout << reuse << std::endl;
    int reusetmp = 1;
    // 强制使用被TIME_WAIT占用的地址
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reusetmp, sizeof(reuse));
    getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, &len);
    std::cout << reuse << std::endl;
    return 0;
}