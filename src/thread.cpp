#include "thread.h"
#include <thread>
/* ----------------- �̷߳���ʵ�� ----------------- */
int Thread::generateId_ = 0;

/* �̹߳��� */
Thread::Thread(ThreadFunc func)
    : func_(func) // ʹ��func_���հ����󶨵��̺߳�������
    , threadId_(generateId_++)
{
}

/* �߳����� */
Thread::~Thread()
{
}

/* �����߳� */
void Thread::start()
{
    /* ����һ���߳���ִ��һ���̺߳��� */
    std::thread t(func_, threadId_);  // ����һ���̶߳���threadId_��ͨ������Ԥ����ʵ�ָ��̺߳������ε�
    /*
     * ���÷����̣߳����̶߳������̺߳������룬��ֹ���ֹ¶�����
     * Linux��ʹ��pthread_detach pthread_t���÷����߳�
     */
    t.detach();
}

/* ��ȡ�߳�id */
int Thread::getId() const
{
    return threadId_;
}