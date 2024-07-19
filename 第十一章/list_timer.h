#ifndef LIST_TIMER
#define LIST_TIMER
#include <time.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const int BUF_SIZE = 64;

class util_timer;
// 用户数据结构
typedef struct 
{
    sockaddr_in address;
    int sock;
    char buf[BUF_SIZE];
    util_timer *timer;
}client_data, *client_data_ptr;

// 定时器类
class util_timer 
{
public:
    util_timer(): next(NULL), prev(NULL) {}

public:
    time_t expire;              // 任务超时时间
    void(*cb_func)(client_data*);   //任务回调函数
    client_data* user_data;
    util_timer* next;
    util_timer* prev;
};

// 定时器链表，是一个按照超时时间升序排列的双向链表
class sort_timer_list
{
public:
    sort_timer_list(): head(NULL), tail(NULL) {}
    ~sort_timer_list()
    {
        // 销毁链表中的每个结点
        util_timer* tmp = head;
        while(tmp)
        {
            head = head->next;
            delete tmp;
            tmp = head;
        }
    }

    // 将目标定时器插入到链表中
    void add_timer(util_timer* node)
    {
        if(!node) return ;
        if((head == NULL) && (tail == NULL))
        {
            head = tail = node;
            return ;
        }
        add_timer_util(node);
    }

    // 调整定时器
    void adjust_timer(util_timer* node)
    {
        if(!node) return ;

        util_timer* tmp = node->next;
        // 不需要调整
        if(!tmp || (node->expire < tmp->expire))
        return ;
        else if(node == head)
        {
            // 先摘下来，然后重新插入
            util_timer* tmp = head;
            head = tmp;
            head->prev = NULL;
            tmp->next == NULL;
            add_timer_util(tmp);
        }
        else
        {
            node->prev->next = tmp;
            tmp->prev = node->prev;
            add_timer_util(node);
        }
    }

    // 删除定时器
    void delete_timer(util_timer* node)
    {
        if(!node) return ;
        if((node == head) && (node == tail))
        {
            delete node;
            head = tail = NULL;
            return ;
        }
        else if(node == head)
        {
            head = head->next;
            head->prev = NULL;
        }
        else if(node == tail)
        {
            tail = node->prev;
            tail->next = NULL;
        }
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        delete node;
    }

    // 定时执行，检查是否需要执行信号处理函数
    void tick()
    {
        if(!head) return ;

        puts("timer tick");
        time_t cur = time(NULL);
        util_timer* tmp = head;

        while(tmp)
        {
            if(!tmp) break;

            if(tmp->expire <= cur)
            tmp->cb_func(tmp->user_data);
            else break;

            delete_timer(tmp);
            tmp = head;
        }
    }
private:
    util_timer* head;
    util_timer* tail;
    void add_timer_util(util_timer* node)
    {
        util_timer* tmp = head;
        while(tmp && tmp->expire < node->expire)
        tmp = tmp->next;
        if(tmp == head)
        {
            node->next = head;
            head->prev = node;
            head = node;
        }
        else if(tmp == NULL)
        {
            tail->next = node;
            node->prev = tail;
            tail = node;
        }
        else
        {
            util_timer* pre_tmp = tmp->prev;
            pre_tmp->next = node;
            node->next = tmp;
            tmp->prev = node;
            node->prev = pre_tmp;
        }
    }
};
#endif