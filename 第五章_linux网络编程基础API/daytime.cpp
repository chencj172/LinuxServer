#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

char buf[1024];
int main(int argc, char* argv[])
{
    assert(argc == 2);
    char *hostname = argv[1];
    // 获取主机信息
    auto hostmsg = gethostbyname(hostname);
    for(int i=0;hostmsg->h_addr_list[i];i++)
    printf("IP addr %d : %s\n", i, inet_ntoa(*(struct in_addr*)hostmsg->h_addr_list[i]));
    // 获取主机服务,都是从/etc/services下获取服务
    auto hostserver = getservbyname("daytime", "tcp");
    printf("daytime server port is : %d\n", ntohs(hostserver->s_port));

    int sock;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock != -1);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    // 端口和ip已经是网络字节序
    addr.sin_addr = *(struct in_addr*)hostmsg->h_addr_list[0];
    addr.sin_port = hostserver->s_port;

    assert((connect(sock, (sockaddr*)&addr, sizeof(sockaddr))) != -1);

    std::cout << "connected...\n";
    memset(buf, 0, sizeof(buf));
    recv(sock, buf, sizeof(buf), 0);
    std::cout << buf << std::endl;
    return 0;
}