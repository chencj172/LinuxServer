#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

void* thread_handle(void *arg)
{
    sigset_t *set = (sigset_t*)arg;

    int s, sig;
    while(1)
    {
        s = sigwait(set, &sig);

        std::cout << "signal handling thread got signal " << sig << std::endl;
        alarm(2);
    }
}
int main(int argc, const char* argv[])
{
    pthread_t thread;
    sigset_t set, set_tmp;

    sigfillset(&set_tmp);
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigdelset(&set_tmp, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    alarm(2);
    std::cout << "ALARM : " << SIGALRM << std::endl;

    pthread_create(&thread, NULL, thread_handle, (void*)&set_tmp);

    pthread_join(thread, NULL);
    return 0;
}