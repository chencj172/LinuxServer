#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER
#include <netinet/in.h>
#include <time.h>
#include <iostream>
#include <chrono>
 
int64_t getCurrentMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis;
}
 
const int BUF_SIZE = 64;

class tw_timer;
// 用户数据结构
typedef struct 
{
    sockaddr_in address;
    int sock;
    char buf[BUF_SIZE];
    tw_timer *timer;
}client_data, *client_data_ptr;

// 定时器结点
class tw_timer
{
public:
    tw_timer(int rot, int slot):prev(NULL), next(NULL), rotation(rot), time_slot(slot) {}
    client_data *clnt_data;
    int rotation;   // 要转多少圈
    int time_slot;  // 对应时间轮哪个槽
    void(*cb_func)(client_data *);
    tw_timer *prev;
    tw_timer *next;
};

// 时间轮
class time_wheel
{
public:
    time_wheel(): cur_slot(0)
    {
        // 初始化链表头结点
        for(int i=0;i<N;i++)
        slots[i] = NULL;
    }
    ~time_wheel()
    {
        for(int i=0;i<N;i++)
        {
            tw_timer *tmp = slots[i];
            while(tmp)
            {
                slots[i] = slots[i]->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }

    // 根据传入的定时时间添加到对应的槽中，并返回该定时器结点
    tw_timer* add_timer(int timeout)
    {
        if(timeout < 0)
        {
            return NULL;
        }
        int ticks;
        if(timeout < SI)
        ticks = 1;
        else 
        ticks = timeout / SI;

        int time_slot = (ticks + cur_slot) % N;
        int rotation = ticks / N;
        tw_timer *timer = new tw_timer(rotation, time_slot);
        int64_t pre_time = getCurrentMillis();
        if(slots[time_slot] == NULL)
        {
            // 当作头结点
            slots[time_slot] = timer;
            timer->next = NULL;
            timer->prev = NULL;
        }
        else
        {
            timer->next = slots[time_slot];
            slots[time_slot]->prev = timer;
            slots[time_slot] = timer;
        }
        add_time += getCurrentMillis() - pre_time;
        tw_timer_count++;
        if(tw_timer_count == 500)
        std::cout << add_time << std::endl;

        std::cout << "add slots is " << time_slot << ",addtime : " << add_time << ",count : " << tw_timer_count << std::endl;

        return timer;
    }

    // 删除目标定时器
    void delete_timer(tw_timer *timer)
    {
        if(!timer) return;
        
        // 如果是头结点
        if(slots[timer->time_slot] == timer)
        {
            slots[timer->time_slot] = slots[timer->time_slot]->next;
            if(slots[timer->time_slot])
            slots[timer->time_slot]->prev = NULL;
        }
        else
        {
            timer->prev->next = timer->next;
            if(timer->next)
            timer->next->prev = timer->prev;
        }
        delete timer;
    }

    // tick函数，SI时间到了之后，时间槽移动一格
    void tick()
    {
        // 先检查有没有到期的定时器结点
        tw_timer *tmp = slots[cur_slot];
        std::cout << "current slot is " << cur_slot << std::endl;
        while(tmp)
        {
            if(tmp->rotation == 0)
            {
                // std::cout << "client disconnected...\n";
                tmp->cb_func(tmp->clnt_data);
                delete_timer(tmp);
                tmp = slots[cur_slot];
            }
            else
            {
                (tmp->rotation)--;
                tmp = tmp->next;
            }
        }
        cur_slot = (cur_slot + 1) % N;
    }
private:
    static const int N = 60; // 槽数
    static const int SI = 1;   // 每隔一秒转动一次
    tw_timer *slots[N];         // 每个槽对应一个无序链表
    int cur_slot;                     // 当前槽

    int64_t add_time = 0;        // 添加所花费的时间
    int tw_timer_count = 0;  // 定时器的个数
};

#endif