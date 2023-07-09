#include "task.h"

/* ----------------- Task实现 ----------------- */
/* 用于设置线程返回值，并多态执行任务 */
Task::Task()
    : result_(nullptr)
{
}

void Task::exec()
{
    if (result_ != nullptr)
    {
        /* 这里多态调用 */
        result_->setVal(run());
    }
}

/* 用于设置任务返回值 */
void Task::setResult(Result* res)
{
    result_ = res;
}

/* ----------------- Result实现 ----------------- */
Result::Result(std::shared_ptr<Task> task, bool isValid)
    : isValid_(isValid), task_(task)
{
    task->setResult(this);
}

// 用户调用get方法用于获取任务执行完的返回值
Any Result::get()
{
    if (!isValid_)
    {
        return "";
    }

    sem_.wait(); // 任务如果没有执行完，会阻塞用户的线程
    return std::move(any_);
}

void Result::setVal(Any any)
{
    /* 存储当前任务task的返回值 */
    this->any_ = std::move(any);
    sem_.post(); // 已经获取到了任务的返回值，增加信号量资源
}