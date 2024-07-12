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
#include <sys/uio.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
    assert(argc == 2);
    int port = atoi(argv[1]);
    int sock;
    sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);
    sock = socket(PF_INET, SOCK_STREAM ,0 );
    assert(sock != -1);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    assert((bind(sock, (sockaddr*)&serv_addr, sizeof(sockaddr))) != -1);
    listen(sock, 5);

    int clnt_sock = accept(sock, (sockaddr*)&clnt_addr, &clnt_len);
    assert(clnt_sock != -1);
    puts("connected...");

    // 创建管道将读入的数据在通过管道传给客户端
    int pipefd[2];
    pipe(pipefd);
    int ret = splice(clnt_sock, NULL, pipefd[1], NULL, 10, SPLICE_F_MORE | SPLICE_F_MOVE);
    std::cout << "ret : " << ret << std::endl;
    assert(ret != -1); 
    ret = splice(pipefd[0], NULL, clnt_sock, NULL, 10, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    close(clnt_sock);
    close(sock);
    return 0;
}