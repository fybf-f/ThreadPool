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

/* ö���̳߳�֧�ֵ�ģʽ:Fixed, Cachedģʽ */
enum class PoolMode
{
    MODE_FIXED,  // �̶��������߳�
    MODE_CACHED, // �߳������ζ�̬����
};

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

    /* �����̳߳�Cachedģʽ���߳���ֵ */
    void setThreadSizeThreshHold(int threshHold);

    /* ���̳߳��ύ���� */
    Result submitTask(std::shared_ptr<Task> sp);

    /* �����̳߳� */
    void start(int initThreadSize = std::thread::hardware_concurrency());  // ��ȡ��ǰϵͳCPU�ĺ�������

    /* ���ÿ������� */
    ThreadPool(const ThreadPool&) = delete;
    /* ��ֹ�� = ��ʽ���ÿ������� */
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    /*
     * �����̺߳�����ThreadPool�̳߳��У�������ʷ���ThreadPool�̳߳غ���
     * ���̺߳�������ִ����������е�����
     */
    void threadFunc(int threadId);

    /* ����̳߳�pool������״̬ */
    bool checkRunningState() const;

private:
    /* �߳���صĳ�Ա���� */
    /* std::vector<std::unique_ptr<Thread>> threads_; // �߳��б�ʹ������ָ���Զ��ͷ���Դ */
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;  // �߳��б�,��vector��Ϊʹ��map�Ƿ�����Cachedģʽ�£�ͨ�������ܻ����߳�
    int initThreadSize_;                           // ��ʼ�߳�����
    int threadSizeThreshHold_;                     // �߳�����������ֵ
    std::atomic_int curThreadSize_;                // ��¼��ǰ�̳߳������̵߳�������
    std::atomic_int idleThreadSize_;               // ��¼�����̵߳�����

    /* ������صĳ�Ա���� */
    std::queue<std::shared_ptr<Task>> taskQue_; // �������
    std::atomic_int taskSize_;                  // ���������
    int taskQueMaxThreshHold_;                  // �������������������ֵ

    /* �̼߳�ͨ����صĳ�Ա���� */
    std::mutex taskQueMtx_;             // ��֤������е��̰߳�ȫ
    std::condition_variable notFull_;   // ��ʾ������в���
    std::condition_variable notEmpty_;  // ��ʾ������в���
    std::condition_variable exitCond_;  // �ȴ��߳���Դȫ������

    /* �̳߳�״̬��صĳ�Ա���� */
    PoolMode poolMode_;              // ��ǰ�̳߳صĹ���ģʽ
    std::atomic_bool isPoolRunning_; // �̳߳��Ƿ��������̳߳ؿ�������ô�Ͳ���ͨ��setMode���������̳߳صĹ���ģʽ
};

#endif
