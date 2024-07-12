#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>

int main(int argc, const char* argv[])
{
    openlog("syslog", LOG_CONS | LOG_PID, 0);
    // 输出调试信息
    syslog(LOG_DEBUG, "no bug");
    closelog();
    return 0;
}