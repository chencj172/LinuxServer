#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <pthread.h>

static pthread_t pids[4];
static int pids_arg[4] = {5, 1, 3, 2};
static sem_t sem;

void* pthread_handle(void *arg)
{
    int* sec = (int *)arg;

    std::cout << *sec << "  begin...\n";
    sem_wait(&sem);
    sleep(*sec);
    sem_post(&sem);
    std::cout << *sec << "  end...\n";

    return nullptr;
}
int main(int argc, const char* argv[])
{
    sem_init(&sem, 0, 2);
    pthread_create(&pids[0], nullptr, pthread_handle, &pids_arg[0]);
    pthread_create(&pids[1], nullptr, pthread_handle, &pids_arg[1]);
    pthread_create(&pids[2], nullptr, pthread_handle, &pids_arg[2]);
    pthread_create(&pids[3], nullptr, pthread_handle, &pids_arg[3]);

    sem_destroy(&sem);
    for(int i=0;i<4;i++)
    pthread_join(pids[i], NULL);

    return 0;
}