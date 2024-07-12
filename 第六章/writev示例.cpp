#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/uio.h>

const int BUF_SIZE = 1024;
static const char* status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char* argv[])
{
    assert(argc == 3);
    int port = atoi(argv[1]);
    char* filename = argv[2];

    int sock;
    sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock != -1);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);

    assert(bind(sock, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(sock, 5) != -1);

    int clnt_sock = accept(sock, (sockaddr*)&clnt_addr, &clnt_len);
    assert(clnt_sock != -1);
    puts("connected...");

    // 存放头部信息
    char header_buf[BUF_SIZE];
    memset(header_buf, 0, BUF_SIZE);
    // 存放文件内容
    char *file_buf;
    // 获取文件属性
    struct stat file_stat;
    bool valid = true;
    // 记录header_buf用了多少空间
    int len = 0;

    if(stat(filename, &file_stat) < 0)
    {
        valid = false;
    }
    else
    {
        if(S_ISDIR(file_stat.st_mode))
        {
            // 是一个目录
            valid = false;
        }
        else if(file_stat.st_mode & S_IROTH)
        {
            // 有读文件的权限
            FILE* fp = fopen(filename, "rb");
            // int fd = open(filename, O_RDONLY);
            file_buf = new char [file_stat.st_size + 1]{'\0'};
            memset(file_buf, 0, file_stat.st_size + 1);
            // read(fd, file_buf, file_stat.st_size);
            fread(file_buf, 1, file_stat.st_size, fp);
            fclose(fp);
        }
        else 
        {
            valid = false;
        }
    }

    if(valid)
    {
        int ret = snprintf(header_buf, BUF_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
        len += ret;
        ret = snprintf(header_buf + len, BUF_SIZE - 1 - len, "Content-Length: %d\r\n", file_stat.st_size);
        len += ret;
        ret = snprintf(header_buf + len, BUF_SIZE - 1 - len, "%s", "\r\n");

        struct iovec iv[2];
        iv[0].iov_base = header_buf;
        iv[0].iov_len = strlen(header_buf);
        iv[1].iov_base = file_buf;
        iv[1].iov_len = file_stat.st_size;
        ret = writev(clnt_sock, iv, 2);
    }
    else
    {
        int ret = snprintf(header_buf, BUF_SIZE - 1, "%s %s\r\n", "HTTP/ 1.1", status_line[1]);
        len += ret;
        ret = snprintf(header_buf + len, BUF_SIZE - 1 - len, "%s", "\r\n");
        send(clnt_sock, header_buf, strlen(header_buf), 0);
    }

    close(clnt_sock);
    close(sock);
    delete [] file_buf;
    return 0;
}