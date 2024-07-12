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

void testbyteorder()
{
    union byteorder
    {
        int num = 0x01020304;
        char union_byte[sizeof(int)];
    }test;

    if(test.union_byte[0] == 1 && test.union_byte[3] == 4)
    std::cout << "大端存储方式\n";
    if(test.union_byte[0] == 4 && test.union_byte[3] == 1)
    std::cout << "小端存储方式\n";

    std::cout << test.union_byte[0] << " "<< (test.union_byte[1]) << " "<< test.union_byte[2] << " "<< test.union_byte[3] << "\n";
    
}

int main(int argc, const char* argv[])
{
    testbyteorder();
    return 0;
}