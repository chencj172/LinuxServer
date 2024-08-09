#include <sys/signal.h>
#include <event.h>

void signal_cb(int fd, short event, void* arg)
{
    struct event_base* base = (event_base*)arg;
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting clearly in two seconds...\n");
    // 中止事件循环
    event_base_loopexit(base, &delay);
}

void timeout_cb(int fd, short event, void* arg)
{
    printf("timeout...\n");
}
int main()
{
    // reactor对象，管理着事件源，基于事件驱动
    struct event_base* base = event_init();

    // 注册信号事件
    struct event* signal_event = evsignal_new(base, SIGINT, signal_cb, base);
    // 会把事件源添加到事件数组中去
    event_add(signal_event, NULL);

    // 注册定时事件
    timeval tv = {1, 0};
    struct event* timeout_event = evtimer_new(base, timeout_cb, NULL);
    event_add(timeout_event, &tv);

    // 循环等待事件的发生
    event_base_dispatch(base);

    event_free(timeout_event);
    event_free(signal_event);
    event_base_free(base);
    return 0;
}