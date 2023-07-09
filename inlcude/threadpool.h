#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <iostream>
#include <unordered_map>
#include <thread>

#include "any.h"
#include "semaphore.h"
#include "task.h"
#include "thread.h"

/* 枚举线程池支持的模式:Fixed, Cached模式 */
enum class PoolMode
{
    MODE_FIXED,  // 固定数量的线程
    MODE_CACHED, // 线程数量课动态增长
};

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

    /* 设置线程池Cached模式下线程阈值 */
    void setThreadSizeThreshHold(int threshHold);

    /* 给线程池提交任务 */
    Result submitTask(std::shared_ptr<Task> sp);

    /* 开启线程池 */
    void start(int initThreadSize = std::thread::hardware_concurrency());  // 获取当前系统CPU的核心数量

    /* 禁用拷贝构造 */
    ThreadPool(const ThreadPool&) = delete;
    /* 禁止以 = 形式调用拷贝构造 */
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    /*
     * 定义线程函数在ThreadPool线程池中，方便访问访问ThreadPool线程池函数
     * 该线程函数用于执行任务队列中的任务
     */
    void threadFunc(int threadId);

    /* 检查线程池pool的运行状态 */
    bool checkRunningState() const;

private:
    /* 线程相关的成员变量 */
    /* std::vector<std::unique_ptr<Thread>> threads_; // 线程列表，使用智能指针自动释放资源 */
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;  // 线程列表,从vector变为使用map是方便在Cached模式下，通过键就能回收线程
    int initThreadSize_;                           // 初始线程数量
    int threadSizeThreshHold_;                     // 线程数量上限阈值
    std::atomic_int curThreadSize_;                // 记录当前线程池里面线程的总数量
    std::atomic_int idleThreadSize_;               // 记录空闲线程的数量

    /* 任务相关的成员变量 */
    std::queue<std::shared_ptr<Task>> taskQue_; // 任务队列
    std::atomic_int taskSize_;                  // 任务的数量
    int taskQueMaxThreshHold_;                  // 任务队列数量的上限阈值

    /* 线程间通信相关的成员变量 */
    std::mutex taskQueMtx_;             // 保证任务队列的线程安全
    std::condition_variable notFull_;   // 表示任务队列不满
    std::condition_variable notEmpty_;  // 表示任务队列不空
    std::condition_variable exitCond_;  // 等待线程资源全部回收

    /* 线程池状态相关的成员变量 */
    PoolMode poolMode_;              // 当前线程池的工作模式
    std::atomic_bool isPoolRunning_; // 线程池是否开启，当线程池开启，那么就不能通过setMode方法更改线程池的工作模式
};

#endif
