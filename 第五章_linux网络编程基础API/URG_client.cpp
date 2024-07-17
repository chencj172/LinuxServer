#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>

int main(int argc, const char* argv[])
{
    int clnt_sock;
    sockaddr_in serv_addr;
    
    assert(argc == 3);
    clnt_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(clnt_sock != -1);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    assert(connect(clnt_sock, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    sleep(1);
    send(clnt_sock, "123", strlen("123"), 0);
    // 发送带外数据，但是只有最后一个被当作带外数据
    send(clnt_sock, "abc", strlen("abc"), MSG_OOB);
    send(clnt_sock, "123", strlen("123"), 0);

    close(clnt_sock);
    return 0;
}