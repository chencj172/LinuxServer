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
#include <sys/shm.h>

// 消费者

union semun 
{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
    struct seminfo* __buf;
};
int create_sem(key_t key)
{
    // 可以结合IPC_EXCL查看当前信号量是否已经存在，存在的话直接获取就行
    int sem_id = semget(key, 1, IPC_CREAT | 0666);

    semun sem_un;
    sem_un.val = 0;
    semctl(sem_id, 0, SETVAL, sem_un);
    return sem_id;
}
int create_shm(key_t key)
{
    int shm_id = shmget(key, 100, IPC_CREAT | 0666);
    return shm_id;
}
void pv(int sem_id, int  op)
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
    key_t sem_key, shm_key;
    sem_key = ftok("sem", 1);
    shm_key = ftok("shm", 2);
    int sem_id = create_sem(sem_key);
    int shm_id = create_shm(shm_key);
    
    for(int i=0;i<5;i++)
    {
        std::cout << "wait producer...\n";

        pv(sem_id, -1);
        char *data = (char*)shmat(shm_id, NULL, 0);
        std::cout << data << std::endl;
        shmdt(data);
    }

    semctl(sem_id, 0, IPC_RMID);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}