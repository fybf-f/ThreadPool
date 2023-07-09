#include "thread.h"
#include <thread>
/* ----------------- 线程方法实现 ----------------- */
int Thread::generateId_ = 0;

/* 线程构造 */
Thread::Thread(ThreadFunc func)
    : func_(func) // 使用func_接收绑定器绑定的线程函数对象
    , threadId_(generateId_++)
{
}

/* 线程析构 */
Thread::~Thread()
{
}

/* 启动线程 */
void Thread::start()
{
    /* 创建一个线程来执行一个线程函数 */
    std::thread t(func_, threadId_);  // 创建一个线程对象，threadId_是通过参数预留符实现给线程函数传参的
    /*
     * 设置分离线程，将线程对象与线程函数分离，防止出现孤儿进程
     * Linux中使用pthread_detach pthread_t设置分离线程
     */
    t.detach();
}

/* 获取线程id */
int Thread::getId() const
{
    return threadId_;
}