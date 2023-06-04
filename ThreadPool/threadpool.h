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

/* ���������� */
class Task
{
public:
    /* �û������Զ��������������ͣ���Task����̳У���дrun������ʵ���Զ���������C++��̬�� */
    virtual void run() = 0;
};

/* ö���̳߳�֧�ֵ�ģʽ:fixed, chachedģʽ */
enum class PoolMode
{
    MODE_FIXED,  // �̶��������߳�
    MODE_CACHED, // �߳������ζ�̬����
};

/* �߳����� */
class Thread
{
public:
    /* �̵߳ĺ����������� */
    using ThreadFunc = std::function<void()>;

    /* �̹߳��죬��Ĭ�ϲ����������̶߳���ʱ��Ҫ����һ���̺߳��� */
    Thread(ThreadFunc func);

    /* �߳����� */
    ~Thread();

    /* �����߳� */ 
    void start();
private:
    ThreadFunc func_;  // �����������͵ĳ�Ա���ڳ�ʼ�������б�ʹ��std::bind()������
};

/* �̳߳����� */
class ThreadPool
{
public:
    /* �̳߳ع��� */
    ThreadPool();
    /* �̳߳����� */
    ~ThreadPool();

    /* �����̳߳صĹ���ģʽ */
    void setMode(PoolMode mode);

    /* ����task�������������ֵ */
    void setTaskQueMaxThreshHold(int threshHold);

    /* ���̳߳��ύ���� */
    void submitTask(std::shared_ptr<Task> sp);

    /* �����̳߳� */
    void start(int initThreadSize = 4);

    /* ���ÿ������� */
    ThreadPool(const ThreadPool&) = delete;
    /* ��ֹ�� = ��ʽ���ÿ������� */
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
      /* 
          * �����̺߳�����ThreadPool�̳߳��У�������ʷ���ThreadPool�̳߳غ���
          * ���̺߳�������ִ����������е�����
      */
    void threadFunc();

private:
    std::vector<std::unique_ptr<Thread>> threads_;  // �߳��б�ʹ������ָ���Զ��ͷ���Դ
    size_t initThreadSize_;         // ��ʼ�߳�����

    std::queue<std::shared_ptr<Task>> taskQue_; // �������
    std::atomic_int taskSize_;                  // ���������
    int taskQueMaxThreshHold_;                  // �������������������ֵ

    std::mutex taskQueMtx_;            // ��֤������е��̰߳�ȫ
    std::condition_variable notFull_;  // ��ʾ������в���
    std::condition_variable notEmpty_; // ��ʾ������в���

    PoolMode poolMode_; // ��ǰ�̳߳صĹ���ģʽ
};

#endif
