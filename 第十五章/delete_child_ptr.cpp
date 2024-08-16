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

class A
{
public:
    A()
    {
        std::cout << "构造\n";
    }
    A(const A& a)
    {
        std::cout << "拷贝构造\n";
    }
    A& operator=(const A& a)
    {
        std::cout << "赋值构造\n";
        return *this;
    }
    ~A()
    {
        std::cout << "析构\n";
    }
};
int main(int argc, const char* argv[])
{
    A *ptr = new A();
    pid_t pid;
    pid = fork();
    if(pid > 0)
    {
        
    }
    else
    {
        sleep(1);
        delete ptr;
        exit(0);
    }

    int stat;
    wait(&stat);
    delete ptr;
    return 0;
}