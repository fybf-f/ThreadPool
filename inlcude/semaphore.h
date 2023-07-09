#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
/* 实现一个信号量使用互斥锁与条件变量实现 */
class Semaphore
{
public:
    /* 构造函数，可以指定信号量资源计数 */
    Semaphore(int limit = 0)
        : resLimit_(limit)
        // , isExit_(false) 
    {
    }

    /* 使用默认析构 */
    ~Semaphore() = default;


    /* Linux上执行的析构函数 */
    /*  ~Semaphore()
    {
        isExit_ = true;
    }
    */

    /* 获取一个信号量资源 */
    void wait()
    {
        /* linux代码
        if (isExit_)  // 如果已经执行完毕，直接return
        {
            return;
        }
        */
        std::unique_lock<std::mutex> lock(mtx_); // 获取一把锁
        /* 等待信号量有资源，没有资源进行阻塞 */
        cond_.wait(lock, [&]() -> bool
            { return resLimit_ > 0; });
        resLimit_--;
    }

    /* 增加一个信号量资源 */
    void post()
    {

        /* linux代码
        if (isExit_)  // 如果已经执行完毕，直接return
        {
            return;
        }
        */
        std::unique_lock<std::mutex> lock(mtx_); // 获取一把锁
        resLimit_++;
        /*
         * 此处在linux下会阻塞是因为linux下的condition_variable的析构函数什么也没做！！！
         * 但是微软做了资源释放
         * 所以在windows上能够运行。Linux下此处状态失效，无故阻塞
         */
        cond_.notify_all(); // 告知其他线程信号量+1 
    }

private:
    // std::atomic_bool isExit_;   // 此成员是用于linux中
    int resLimit_;                 // 资源计数
    std::mutex mtx_;               // 互斥锁
    std::condition_variable cond_; // 条件变量
};

#endif // !SEMAPHORE_H