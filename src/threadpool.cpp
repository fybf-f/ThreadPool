#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = INT32_MAX;  // 任务队列的最大长度
const int THREAD_MAX_THRESHHOLD = 200; // 线程创建的最大数量
const int THREAD_MAX_IDLE_TIME = 60;   // 线程最大空闲时间60，单位 s

/* 线程池构造函数(使用初始化参数列表) */
ThreadPool::ThreadPool()
    : initThreadSize_(0) // 初始线程数量,一般使用fixed模式需要获取cpu核心数量
    , taskSize_(0) // 任务数量
    , idleThreadSize_(0) // 尚未创建线程，所以空闲线程数量为0
    , curThreadSize_(0), taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD) // 任务数量上限阈值 
    , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD), poolMode_(PoolMode::MODE_FIXED) // 线程池工作模式，默认使用Fixed工作模式
    , isPoolRunning_(false)
{}

/* 线程池析构 */
ThreadPool::~ThreadPool()
{
    /* 线程池对象在结束后自动执行析构，在线程池析构前，应该把线程池中的线程进行回收 */
    isPoolRunning_ = false;
    /* 回收线程前需要将线程进行唤醒 */
    
    /* 等待线程池里面的所有线程结束返回，此时线程池中的线程有两种状态: 阻塞和执行任务中 */
    std::unique_lock <std::mutex> lock(taskQueMtx_);
    notEmpty_.notify_all();  // 把等待在notEmpty_上的线程唤醒，通知条件变量wait的地方，可以起来干活了
    /* 此处可能会产生阻塞，偶尔出现死锁问题 */
    exitCond_.wait(lock, [&]()->bool { return threads_.size() == 0; });  // 没有线程执行任务threads_size() > 0会阻塞
}

/* 设置线程池的工作模式 */
void ThreadPool::setMode(PoolMode mode)
{
    if (checkRunningState())
    {
        return; // 线程池已经开启，不能更改线程池的工作模式
    }

    poolMode_ = mode;
}

/* 设置task任务队列上限阈值 */
void ThreadPool::setTaskQueMaxThreshHold(int threshHold)
{
    if (checkRunningState())
    {
        return; // 线程池已经开启，不能更改线程池的任务队列上限
    }
    taskQueMaxThreshHold_ = threshHold;
}

/* 设置Cached模式下线程的阈值 */
void ThreadPool::setThreadSizeThreshHold(int threshHold)
{
    if (checkRunningState())
    {
        return; // 线程池已经开启，不能更改线程池的线程数量阈值
    }

    if (poolMode_ == PoolMode::MODE_CACHED)
    {
        threadSizeThreshHold_ = threshHold; // 只有在Cached工作模式下才能更改线程数量阈值
    }
}

/* 给线程池提交任务（重难点）用户调用该接口，传入任务对象，生产任务 */
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    /* 1.获取锁，因为队列不是线程安全的，一边用户线程添加任务，另一边线程池处理任务，需要互斥 */
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    /* 2.线程的通信 等待任务队列有空余 */
    /*
     * 保证任务队列不满，如果满就在此进入阻塞
     * std::condition_variable notFull_;  // 表示任务队列不满
     * notFull_.wait();第一个参数传入智能锁(unique_lock)，第二个参数传入函数，这里使用lambda表达式，并使用值捕获
     * 并且第二个参数返回false才会阻塞
     * 用户提交任务，最长时间不能阻塞超过1s，否则判断任务提交失败，返回
     * wait_for()首先只要条件满足立刻返回，其次会在等待1s钟时恰好条件满足返回，最后超时返回
     * wait_for()返回值：满足条件返回true，等待超时false
     */
    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]() -> bool
        { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
    {
        // 表示nut_Full_等待1s钟，条件依然没有满足
        std::cerr << "task queue is full, submit task fail." << std::endl;

        /*
         * 错误，返回Result对象，两种方式进行返回
         * return task->getResult();  此种返回方式是把Result对象封装进Task中，但是不行，不合理。
         * 线程执行完task，task对象就被析构，依赖于task对象的Result也没了
         * 此时通过Result对象进行获取返回值就会报错
         */
        return Result(sp, false); // 正确的返回方式。任务提交失败说明Result的返回方式无效
    }

    /* 3.如果有空余，把任务放到任务队列中 */
    /*
     * 将任务对象放到任务队列中，如果任务队列满的话，上一步wait释放锁，
     * 锁会立即被线程池中的线程抢占，进行任务处理
     * 对临界区taskQue_的操作会因为没有锁而导致阻塞，直到其他线程释放锁或者任务队列为空
     */
    taskQue_.emplace(sp);
    taskSize_++; // std::atomic_int taskSize_; 原子类型

    /* 4.因为新放了任务，任务队列肯定不空，在notEmpty_上进行通知 */
    notEmpty_.notify_all(); // 生产完任务，任务队列不空，通知线程池分配线程执行任务

    /*
     * Cached模式需要根据任务数量和空闲线程地数量判断是否需要新的线程出来
     * 适用场景：小而快的任务
     */

     /* Cached工作模式下创建新的线程对象 */
    if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < threadSizeThreshHold_)
    {
        /* 满足Cached模式，创建新线程 */
        /* auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this)); */
        /* threads_.emplace_back(std::move(ptr)); */
        std::cout << ">>>create new thread..." << std::endl;

        /* 创建unique_ptr管理Thread，自动释放Thread, std::std::placeholders::_1是绑定函数的参数占位符 */
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();  // 获取线程id
        threads_.emplace(threadId, std::move(ptr));  // 把线程添加到线程池中
        threads_[threadId]->start();  // 启动线程，并不是让线程立刻去执行任务
        curThreadSize_++;  // 修改线程数量相关的变量
        idleThreadSize_++;  // 新建的线程是空闲线程，需要+1
    }

    /* 5.返回任务的Result对象 */
    return Result(sp); // 返回右值，应该匹配右值引用的构造函数
}

/* 开启线程池（开启线程池是默认初始化默认线程数量） */
void ThreadPool::start(int initThreadSize)
{
    /* 设置线程池为开启状态 */
    isPoolRunning_ = true;

    /* 记录初始线程个数 */
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;

    /* 创建线程对象 */
    for (int i = 0; i < initThreadSize_; i++)
    {
        /*
         * 创建thread线程对象的时候，使用绑定器把线程函数给到thread线程对象
         * 使得创建线程对象时自动执行threadFunc线程函数
         * 使用绑定器实现，将线程对象绑定到线程池上
         * this表示的是ThreadPool,而不是Thread
         */
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)); // 创建unique_ptr管理Thread，自动释放Thread
        int threadId = ptr->getId();  // 获取线程id
        threads_.emplace(threadId, std::move(ptr));  // 把线程添加到线程池中

        /* threads_.emplace(std::move(ptr)); // 不使用std::move资源转移会导致容器底层调用拷贝构造去拷贝unique_ptr导致报错 */
    }

    /* 启动所有线程std::vector<std::unique_ptr<Thread>> threads_ */
    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start(); // 需要执行一个线程函数，在创建对象时进行绑定
        idleThreadSize_++;    // 每启动一个线程，初始的空闲的线程数量就+1
    }
}

/* 定义线程函数 线程池里面的所有线程从任务队列里面消费任务，线程函数返回，相应的线程也就结束了 */
void ThreadPool::threadFunc(int threadId)
{
    auto lastTime = std::chrono::high_resolution_clock().now();  // 获取线程刚开始执行任务的时间，高精度

    /* 所有任务必须执行完成，线程池才可以回收所有线程资源 */
    for (;;)
    {
        std::shared_ptr<Task> task; // 用于从下面的作用域中取出任务队列中的任务，基类指针
        /* 一个作用域 */
        {
            /* 1.先获取锁 */
            std::unique_lock<std::mutex> lock(taskQueMtx_);  // threadpool对象析构时，也会获取这把锁 

            std::cout << "tid:" << std::this_thread::get_id()
                << "尝试获取任务..." << std::endl;
            /* 2.等待notEmpty_条件 */
            /*
             * taskQue_size() == 0说明任务队列为空，lambda表达式返回false，进行阻塞
             * taskQue_size() > 0 时不会阻塞，成功加锁
             */
            
            /*
             * Cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s应该把多余的线程结束回收掉
             * 结束回收掉（超过initThreadSize_数量的线程要进行回收）
             * 当前时间 - 上一次线程执行的时间 > 60s 也需要回收时间
             */
            
            /* 每一秒返回一次，怎么区分超时返回还是有任务待执行返回 */
            while (taskQue_.size() == 0)  // 锁 + 双重判断解决死锁,获取锁前进行判断，获得锁后也进行判断保证线程安全
            {
                if (!isPoolRunning_)  
                {
                    /* 线程池结束，就进行线程回收。 */
                    threads_.erase(threadId);
                    exitCond_.notify_all();
                    std::cout << "threadid:" << std::this_thread::get_id()
                        << "exit" << std::endl;
                    return;  // 线程函数结束，线程结束
                }

                if (poolMode_ == PoolMode::MODE_CACHED)
                {
                    /* 条件变量超时返回 */
                    if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                    {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if (dur.count() >= THREAD_MAX_IDLE_TIME
                            && curThreadSize_ > initThreadSize_)  // 这里虽然超时60s但是不要有魔鬼数字，使用const常量进行替换
                        {
                            /*
                             * 开始回收当前线程所作的工作:
                             * 记录线程数量相关的变量的值进行修改
                             * 把线程对象从线程列表容器中删除(std::vector<std::unique_ptr<Thread>> threads_;)
                             * 当前没有办法知道threadFunc线程函数匹配哪一个thread线程对象
                             * 通过thread_tid线程id找到线程，进行删除
                             */
                            threads_.erase(threadId);   
                            curThreadSize_--;
                            idleThreadSize_--;
                            std::cout << "threadid:" << std::this_thread::get_id()
                                << "exit" << std::endl;
                            exitCond_.notify_all();
                            return;
                        }
                           
                    }
                    
                }
                else
                {
                    /* 等待notEmpty_条件 */
                    notEmpty_.wait(lock);
                }

                /* 线程池要结束，回收线程资源 */
                /*
                 * while循环时已经判断线程池是否运行了，退出while循环进行回收 
                if (!isPoolRunning_)
                {
                    threads_.erase(threadId);
                    std::cout << "threadid:" << std::this_thread::get_id()
                        << "exit" << std::endl;
                    exitCond_.notify_all();
                    return;  // 结束线程函数，就是结束当前线程了
                }
                 */
            }


            idleThreadSize_--;

            std::cout << "tid:" << std::this_thread::get_id()
                << "获取任务成功." << std::endl;

            /* 3.从任务队列中取出一个任务来 */
            task = taskQue_.front(); // 获取队头元素
            taskQue_.pop();          // 将取出的任务从任务队列中删除
            taskSize_--;

            /* 如果依然有剩余任务，继续通知其他线程执行任务 */
            if (taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }

            /* 4.取出一个任务需要进行通知 */
            notFull_.notify_all(); // 取出一个任务表示任务队列不满，使用notFull_进行通知生产任务

            /* 5.成功取出任务应该释放锁 */
            /* 超出作用域智能锁会自动释放锁 */
        }

        /* .当前线程负责执行这个任务 */
        if (task != nullptr)
        {
            // task->run();  // 执行任务；把任务的返回值通过setVal方法给到Result对象
            task->exec();
        }

        /* 线程执行完任务，空闲线程的数量就+1 */
        idleThreadSize_++;

        lastTime = std::chrono::high_resolution_clock().now();  // 更新线程执行完任务的时间
    }

}

/* 检查线程池pool工作状态 */
bool ThreadPool::checkRunningState() const
{
    return isPoolRunning_;
}
