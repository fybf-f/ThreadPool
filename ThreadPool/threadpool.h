#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <iostream>

/* 任务抽象基类 */
class Task
{
public:
    /* 用户可以自定义任意任务类型，从Task基类继承，重写run方法，实现自定义任务处理（C++多态） */
    virtual void run() = 0;
};

/* 枚举线程池支持的模式:fixed, chached模式 */
enum class PoolMode
{
    MODE_FIXED,  // 固定数量的线程
    MODE_CACHED, // 线程数量课动态增长
};

/* 线程类型 */
class Thread
{
public:
    /* 线程的函数对象类型 */
    using ThreadFunc = std::function<void()>;

    /* 线程构造，非默认参数，构造线程对象时需要传入一个线程函数 */
    Thread(ThreadFunc func);

    /* 线程析构 */
    ~Thread();

    /* 启动线程 */ 
    void start();
private:
    ThreadFunc func_;  // 函数对象类型的成员，在初始化参数列表使用std::bind()绑定器绑定
};

/* 线程池类型 */
class ThreadPool
{
public:
    /* 线程池构造 */
    ThreadPool();
    /* 线程池析构 */
    ~ThreadPool();

    /* 设置线程池的工作模式 */
    void setMode(PoolMode mode);

    /* 设置task任务队列上限阈值 */
    void setTaskQueMaxThreshHold(int threshHold);

    /* 给线程池提交任务 */
    void submitTask(std::shared_ptr<Task> sp);

    /* 开启线程池 */
    void start(int initThreadSize = 4);

    /* 禁用拷贝构造 */
    ThreadPool(const ThreadPool&) = delete;
    /* 禁止以 = 形式调用拷贝构造 */
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
      /* 
          * 定义线程函数在ThreadPool线程池中，方便访问访问ThreadPool线程池函数
          * 该线程函数用于执行任务队列中的任务
      */
    void threadFunc();

private:
    std::vector<std::unique_ptr<Thread>> threads_;  // 线程列表，使用智能指针自动释放资源
    size_t initThreadSize_;         // 初始线程数量

    std::queue<std::shared_ptr<Task>> taskQue_; // 任务队列
    std::atomic_int taskSize_;                  // 任务的数量
    int taskQueMaxThreshHold_;                  // 任务队列数量的上限阈值

    std::mutex taskQueMtx_;            // 保证任务队列的线程安全
    std::condition_variable notFull_;  // 表示任务队列不满
    std::condition_variable notEmpty_; // 表示任务队列不空

    PoolMode poolMode_; // 当前线程池的工作模式
};

#endif
