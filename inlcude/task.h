#ifndef TASK_H
#define TASK_H
#include "any.h"
#include "semaphore.h"
#include <memory>

class Result;
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
    Result* result_; // 此处不能使用智能指针，否则出现智能指针的相互引用，导致资源永远得不到释放，Result对象的声明周期比Task长
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
    Any any_;                    // 存储任务的返回值
    Semaphore sem_;              // 线程通信的信号量，告知任务是否执行完
    std::shared_ptr<Task> task_; // 指向对应获取返回值的任务对象
    std::atomic_bool isValid_;   // 检查任务的返回值是否有效
};

#endif // !TASK_H
