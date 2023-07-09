#ifndef THREAD_H
#define THREAD_H
#include <functional>

/* 线程类型 */
class Thread
{
public:
    /* 线程的函数对象类型 */
    using ThreadFunc = std::function<void(int)>;

    /* 线程构造，非默认参数，构造线程对象时需要传入一个线程函数 */
    Thread(ThreadFunc func);

    /* 线程析构 */
    ~Thread();

    /* 启动线程 */
    void start();

    /* 获取线程id */
    int getId() const;

private:
    ThreadFunc func_;        // 函数对象类型的成员，在初始化参数列表使用std::bind()绑定器绑定
    static int generateId_;  // 初始为0，用于初始化threadId_线程id，每次初始化完都自增
    int threadId_;           // 保存线程id，在Cached模式下用于删除线程对象，这个线程id是我们创建线程的编号，不是在pcb里面记录那个
};

#endif // !THREAD_H
