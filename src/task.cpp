#include "task.h"

/* ----------------- Taskʵ�� ----------------- */
/* ���������̷߳���ֵ������ִ̬������ */
Task::Task()
    : result_(nullptr)
{
}

void Task::exec()
{
    if (result_ != nullptr)
    {
        /* �����̬���� */
        result_->setVal(run());
    }
}

/* �����������񷵻�ֵ */
void Task::setResult(Result* res)
{
    result_ = res;
}

/* ----------------- Resultʵ�� ----------------- */
Result::Result(std::shared_ptr<Task> task, bool isValid)
    : isValid_(isValid), task_(task)
{
    task->setResult(this);
}

// �û�����get�������ڻ�ȡ����ִ����ķ���ֵ
Any Result::get()
{
    if (!isValid_)
    {
        return "";
    }

    sem_.wait(); // �������û��ִ���꣬�������û����߳�
    return std::move(any_);
}

void Result::setVal(Any any)
{
    /* �洢��ǰ����task�ķ���ֵ */
    this->any_ = std::move(any);
    sem_.post(); // �Ѿ���ȡ��������ķ���ֵ�������ź�����Դ
}