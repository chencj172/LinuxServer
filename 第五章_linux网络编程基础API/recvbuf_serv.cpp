#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>

static int clnt_sock;
const int buf_size = 1024;
char buf[buf_size];

int main(int argc, const char* argv[])
{
    int serv_sock;
    sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);

    assert(argc == 2);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(serv_sock != -1);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family =  AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 设置接收缓冲区，但是最小值是2304
    int recv_buf = 50;
    socklen_t recv_buf_len = sizeof(recv_buf);
    setsockopt(serv_sock, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));

    assert(bind(serv_sock, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(serv_sock, 5) != -1);
    clnt_sock = accept(serv_sock, (sockaddr*)&clnt_addr, &clnt_len);
    getsockopt(serv_sock, SOL_SOCKET, SO_RCVBUF, &recv_buf, &recv_buf_len);
    std::cout << "recv buf size : " << recv_buf << std::endl;

    int recv_len = 0;
    int recv_len_tmp;
    while((recv_len_tmp = recv(clnt_sock, buf, buf_size, 0)) > 0)
    {
        recv_len += recv_len_tmp;
    }
    std::cout << "recv_len : " << recv_len << std::endl;

    sleep(10);
    close(clnt_sock);
    close(serv_sock);
    return 0;
}