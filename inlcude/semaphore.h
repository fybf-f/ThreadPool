#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
/* ʵ��һ���ź���ʹ�û���������������ʵ�� */
class Semaphore
{
public:
    /* ���캯��������ָ���ź�����Դ���� */
    Semaphore(int limit = 0)
        : resLimit_(limit)
        // , isExit_(false) 
    {
    }

    /* ʹ��Ĭ������ */
    ~Semaphore() = default;


    /* Linux��ִ�е��������� */
    /*  ~Semaphore()
    {
        isExit_ = true;
    }
    */

    /* ��ȡһ���ź�����Դ */
    void wait()
    {
        /* linux����
        if (isExit_)  // ����Ѿ�ִ����ϣ�ֱ��return
        {
            return;
        }
        */
        std::unique_lock<std::mutex> lock(mtx_); // ��ȡһ����
        /* �ȴ��ź�������Դ��û����Դ�������� */
        cond_.wait(lock, [&]() -> bool
            { return resLimit_ > 0; });
        resLimit_--;
    }

    /* ����һ���ź�����Դ */
    void post()
    {

        /* linux����
        if (isExit_)  // ����Ѿ�ִ����ϣ�ֱ��return
        {
            return;
        }
        */
        std::unique_lock<std::mutex> lock(mtx_); // ��ȡһ����
        resLimit_++;
        /*
         * �˴���linux�»���������Ϊlinux�µ�condition_variable����������ʲôҲû��������
         * ����΢��������Դ�ͷ�
         * ������windows���ܹ����С�Linux�´˴�״̬ʧЧ���޹�����
         */
        cond_.notify_all(); // ��֪�����߳��ź���+1 
    }

private:
    // std::atomic_bool isExit_;   // �˳�Ա������linux��
    int resLimit_;                 // ��Դ����
    std::mutex mtx_;               // ������
    std::condition_variable cond_; // ��������
};

#endif // !SEMAPHORE_H