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

/* Any类型：表示可以接收任意数据的类型 */
class Any
{
public:
    /* 使用默认构造函数能够减少错误 */
    Any() = default;

    /* 使用默认析构函数能够减少错误 */
    ~Any() = default;

    /* 因为使用了unique_ptr禁用左值引用的拷贝构造与赋值，在此禁用 */
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    /* 因为使用了unique_ptr保留了右值引用的赋值，在此默认使用 */
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    // 这个构造函数可以让Any类型接收任意其它的数据
    // 构造Any对象时就让base_基类指针指向派生类对象发生多态，并传入数据
    template<typename T>  // T:int    Derive<int>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {}

    // 这个方法能把Any对象里面存储的data数据提取出来
    template<typename T>
    T cast_()
    {
        /* 通过base_找到他所指向的Derive对象，从中提取data_成员变量 */
        /* 基类指针需要转换成派生类指针，RTTI类型识别（强制类型转换） */
        /* get函数：返回智能指针中保存的裸指针。考虑到有些函数的参数需要内置的裸指针，所以引入该函数。 */
        Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
        if (pd == nullptr)
        {
            throw "type is unmatch!";
        }
        return pd->data_;
    }
private:
    /* 基类类型 */
    class Base
    {
    public:
        virtual ~Base() = default;  // 虚析构，使用默认实现
    };


    // 派生类类型
    /* 派生类类型，保存任意类型的数据，通过多态访问被保存的数据 */
    template<typename T>
    class Derive : public Base
    {
    public:
        /* 将任意类型的数据保存在派生类中，并且发生多态时，需要传入这个任意类型的数据 */
        Derive(T data) : data_(data)
        {}
        T data_;  // 保存了任意的其他类型
    };

private:
    /*
        * 定义一个基类指针用于多态访问任意类型数据
        * unique_ptr把左值引用的拷贝构造函数与赋值删除了，保留了右值引用的拷贝构造与赋值
    */
    std::unique_ptr<Base> base_;
};

/* 声明Task类，Result对象需要绑定一个Task对象 */
class Task;

/* 实现一个信号量使用互斥锁与条件变量实现 */
class Semaphore
{
public:
    /* 构造函数，可以指定信号量资源计数 */
    Semaphore(int limit = 0)
        : resLimit_(limit)
    {}

    /* 使用默认析构 */
    ~Semaphore() = default;

    /* 获取一个信号量资源 */
    void wait()
    {
        std::unique_lock<std::mutex>lock(mtx_);  // 获取一把锁
        /* 等待信号量有资源，没有资源进行阻塞 */
        cond_.wait(lock, [&]()->bool { return resLimit_ > 0; });
        resLimit_--;
    }

    /* 增加一个信号量资源 */
    void post()
    {
        std::unique_lock<std::mutex>lock(mtx_);  // 获取一把锁
        resLimit_++;
        cond_.notify_all();  // 告知其他线程信号量+1
    }


private:
    int resLimit_;  // 资源计数
    std::mutex mtx_;  // 互斥锁
    std::condition_variable cond_;  // 条件变量 
};

/* 实现接收提交到线程池的task任务执行完成后的返回值类型Result，即获得执行完成被提交任务的线程的返回值 */
class Result
{
public:
    /* 构造函数，默认任务正常提交并执行，返回值有效 */
    Result(std::shared_ptr<Task> task, bool isValid = true);

    /* 析构函数，使用只能指针，默认析构函数 */
    ~Result() = default;

    /* 问题一：setVal方法，获取任务执行完的返回值 */
    void setVal(Any any);

    /* 问题二：用户调用这个方法获取task的返回值 */
    Any get();

private:
    Any any_;  // 存储任务的返回值
    Semaphore sem_;  // 线程通信的信号量，告知任务是否执行完   
    std::shared_ptr<Task> task_;  // 指向对应获取返回值的任务对象
    std::atomic_bool isValid_;  // 检查任务的返回值是否有效
};


/* 任务抽象基类 */
class Task
{
public:
    /* 构造函数 */
    Task();

    /* 默认析构函数 */
    ~Task() = default;

    /* 调用setResult方法 */
    void exec();

    /* 设置任务返回值，从run函数中获得 */
    void setResult(Result* res);

    /* 用户可以自定义任意任务类型，从Task基类继承，重写run方法，实现自定义任务处理（C++多态） */
    virtual Any run() = 0;

private:
    Result* result_;  // 此处不能使用智能指针，否则出现智能指针的相互引用，导致资源永远得不到释放，Result对象的声明周期比Task长
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
/*
* example:
    ThreadPool pool;
    pool.start(4);
    class MyTask : public Task
    {
    public:
        void run(){ // 线程代码 }
    };
    pool.submitTask(std::make_shared<MyTask>());
*/
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
    Result submitTask(std::shared_ptr<Task> sp);

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
