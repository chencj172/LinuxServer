#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>

const int BUF_SIZE = 1024;
static int connfd = -1;
void sig_handler(int sig)
{
    int old_erron = errno;
    char sig_buf[BUF_SIZE];
    memset(sig_buf, 0 , sizeof(sig_buf));
    int len = recv(connfd, sig_buf, sizeof(sig_buf), MSG_OOB);
    std::cout << "get " << len << " bytes of oob data : " << sig_buf << std::endl;
    errno = old_erron;
}
void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags |= SA_RESTART;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) != -1);
    assert(listen(listenfd, 5) != -1);
    
    sockaddr_in clnt_addr;
    socklen_t len = sizeof(clnt_addr);
    connfd = accept(listenfd, (sockaddr*)&clnt_addr, &len);
    assert(connfd != -1);
    addsig(SIGURG);
    fcntl(connfd, F_SETOWN, getpid());

    char buf[BUF_SIZE];
    while(1)
    {
        memset(buf, 0, sizeof(buf));
        int len = recv(connfd, buf, sizeof(buf), 0);
        if(len == 0) break;
        std::cout << "get " << len << " bytes of normal data : " << buf << std::endl;
    }
    close(connfd);
    close(listenfd);
    return 0;   
}