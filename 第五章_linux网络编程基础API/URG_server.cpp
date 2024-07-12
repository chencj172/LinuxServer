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
char buf[20];

void isOOB()
{
    std::cout << (sockatmark(clnt_sock) == 1 ? "OOB\n" : "not OOB\n");
}

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

    assert(bind(serv_sock, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(serv_sock, 5) != -1);
    clnt_sock = accept(serv_sock, (sockaddr*)&clnt_addr, &clnt_len);

    memset(buf, 0, sizeof(buf));
    isOOB();
    int recv_len = recv(clnt_sock, buf, 20, 0);
    printf("get %d bytes of normal data : %s\n", recv_len, buf);

    memset(buf, 0, sizeof(buf));
    isOOB();
    recv_len = recv(clnt_sock, buf, 20, MSG_OOB);
    printf("get %d bytes of OOB data : %s\n", recv_len, buf);

    memset(buf, 0, sizeof(buf));
    isOOB();
    recv_len = recv(clnt_sock, buf, strlen("123"), 0);
    printf("get %d bytes of normal data : %s\n", recv_len, buf);

    close(clnt_sock);
    close(serv_sock);
    return 0;
}