#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/sem.h>
#include <sys/wait.h>

union semun
{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
    struct seminfo* __buf;
};

void pv(int sem_id, int op)
{
    struct sembuf sem_b;
    sem_b.sem_op = op;
    // 编号
    sem_b.sem_num = 0;
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
}
int main(int argc, const char* argv[])
{
    int sem_id;
    key_t key;
    key = ftok(".", 100);

    // 创建一个信号量集，该集合中有一个信号量
    sem_id = semget(key, 1, IPC_CREAT | 0666);

    // 设置信号量的值
    union semun sem_un;
    sem_un.val = 1;
    // 将信号量集sem_id中编号为0的信号量值设为1
    semctl(sem_id, 0, SETVAL, sem_un);
    
    pid_t pid;
    pid = fork();
    if(pid == 0)
    {
        //子进程
        pv(sem_id, -1);
        std::cout << "child get the sem and would release it after 5sec\n";
        sleep(5);
        pv(sem_id, 1);
        exit(0);
    }
    else
    {
        // 父进程
        pv(sem_id, -1);
        std::cout << "father get the sem and would release it after 5sec\n";
        sleep(5);
        pv(sem_id, 1);
    }
    
    // 阻塞等待子进程退出
    waitpid(pid, nullptr, 0);
    semctl(sem_id, 0, IPC_RMID, sem_un);
    return 0;
}