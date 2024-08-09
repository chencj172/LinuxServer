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

// 生产者

union semun 
{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
    struct seminfo* __buf;
};
int create_sem(key_t key)
{
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

char* buf[] = {"1", "2", "3", "4", "5"};
int main(int argc, const char* argv[])
{
    key_t sem_key, shm_key;
    sem_key = ftok("sem", 1);
    shm_key = ftok("shm", 2);
    int sem_id = create_sem(sem_key);
    // 创建共享内存
    int shm_id = create_shm(shm_key);
    
    sleep(5);
    for(int i=0;i<5;i++)
    {
        // 共享内存关联到进程地址空间
        char *data = (char*)shmat(shm_id, NULL, 0);
        // 往共享内存中写入数据
        strncpy(data, buf[i], strlen(buf[i]));
        // 分离
        shmdt(data);

        pv(sem_id, 1);
        sleep(5);
    }

    semctl(sem_id, 0, IPC_RMID);
    // 删除共享内存
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}