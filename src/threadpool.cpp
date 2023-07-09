#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = INT32_MAX;  // ������е���󳤶�
const int THREAD_MAX_THRESHHOLD = 200; // �̴߳������������
const int THREAD_MAX_IDLE_TIME = 60;   // �߳�������ʱ��60����λ s

/* �̳߳ع��캯��(ʹ�ó�ʼ�������б�) */
ThreadPool::ThreadPool()
    : initThreadSize_(0) // ��ʼ�߳�����,һ��ʹ��fixedģʽ��Ҫ��ȡcpu��������
    , taskSize_(0) // ��������
    , idleThreadSize_(0) // ��δ�����̣߳����Կ����߳�����Ϊ0
    , curThreadSize_(0), taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD) // ��������������ֵ 
    , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD), poolMode_(PoolMode::MODE_FIXED) // �̳߳ع���ģʽ��Ĭ��ʹ��Fixed����ģʽ
    , isPoolRunning_(false)
{}

/* �̳߳����� */
ThreadPool::~ThreadPool()
{
    /* �̳߳ض����ڽ������Զ�ִ�����������̳߳�����ǰ��Ӧ�ð��̳߳��е��߳̽��л��� */
    isPoolRunning_ = false;
    /* �����߳�ǰ��Ҫ���߳̽��л��� */
    
    /* �ȴ��̳߳�����������߳̽������أ���ʱ�̳߳��е��߳�������״̬: ������ִ�������� */
    std::unique_lock <std::mutex> lock(taskQueMtx_);
    notEmpty_.notify_all();  // �ѵȴ���notEmpty_�ϵ��̻߳��ѣ�֪ͨ��������wait�ĵط������������ɻ���
    /* �˴����ܻ����������ż�������������� */
    exitCond_.wait(lock, [&]()->bool { return threads_.size() == 0; });  // û���߳�ִ������threads_size() > 0������
}

/* �����̳߳صĹ���ģʽ */
void ThreadPool::setMode(PoolMode mode)
{
    if (checkRunningState())
    {
        return; // �̳߳��Ѿ����������ܸ����̳߳صĹ���ģʽ
    }

    poolMode_ = mode;
}

/* ����task�������������ֵ */
void ThreadPool::setTaskQueMaxThreshHold(int threshHold)
{
    if (checkRunningState())
    {
        return; // �̳߳��Ѿ����������ܸ����̳߳ص������������
    }
    taskQueMaxThreshHold_ = threshHold;
}

/* ����Cachedģʽ���̵߳���ֵ */
void ThreadPool::setThreadSizeThreshHold(int threshHold)
{
    if (checkRunningState())
    {
        return; // �̳߳��Ѿ����������ܸ����̳߳ص��߳�������ֵ
    }

    if (poolMode_ == PoolMode::MODE_CACHED)
    {
        threadSizeThreshHold_ = threshHold; // ֻ����Cached����ģʽ�²��ܸ����߳�������ֵ
    }
}

/* ���̳߳��ύ�������ѵ㣩�û����øýӿڣ�������������������� */
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    /* 1.��ȡ������Ϊ���в����̰߳�ȫ�ģ�һ���û��߳����������һ���̳߳ش���������Ҫ���� */
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    /* 2.�̵߳�ͨ�� �ȴ���������п��� */
    /*
     * ��֤������в�������������ڴ˽�������
     * std::condition_variable notFull_;  // ��ʾ������в���
     * notFull_.wait();��һ����������������(unique_lock)���ڶ����������뺯��������ʹ��lambda���ʽ����ʹ��ֵ����
     * ���ҵڶ�����������false�Ż�����
     * �û��ύ�����ʱ�䲻����������1s�������ж������ύʧ�ܣ�����
     * wait_for()����ֻҪ�����������̷��أ���λ��ڵȴ�1s��ʱǡ���������㷵�أ����ʱ����
     * wait_for()����ֵ��������������true���ȴ���ʱfalse
     */
    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]() -> bool
        { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
    {
        // ��ʾnut_Full_�ȴ�1s�ӣ�������Ȼû������
        std::cerr << "task queue is full, submit task fail." << std::endl;

        /*
         * ���󣬷���Result�������ַ�ʽ���з���
         * return task->getResult();  ���ַ��ط�ʽ�ǰ�Result�����װ��Task�У����ǲ��У�������
         * �߳�ִ����task��task����ͱ�������������task�����ResultҲû��
         * ��ʱͨ��Result������л�ȡ����ֵ�ͻᱨ��
         */
        return Result(sp, false); // ��ȷ�ķ��ط�ʽ�������ύʧ��˵��Result�ķ��ط�ʽ��Ч
    }

    /* 3.����п��࣬������ŵ���������� */
    /*
     * ���������ŵ���������У��������������Ļ�����һ��wait�ͷ�����
     * �����������̳߳��е��߳���ռ������������
     * ���ٽ���taskQue_�Ĳ�������Ϊû����������������ֱ�������߳��ͷ��������������Ϊ��
     */
    taskQue_.emplace(sp);
    taskSize_++; // std::atomic_int taskSize_; ԭ������

    /* 4.��Ϊ�·�������������п϶����գ���notEmpty_�Ͻ���֪ͨ */
    notEmpty_.notify_all(); // ����������������в��գ�֪ͨ�̳߳ط����߳�ִ������

    /*
     * Cachedģʽ��Ҫ�������������Ϳ����̵߳������ж��Ƿ���Ҫ�µ��̳߳���
     * ���ó�����С���������
     */

     /* Cached����ģʽ�´����µ��̶߳��� */
    if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < threadSizeThreshHold_)
    {
        /* ����Cachedģʽ���������߳� */
        /* auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this)); */
        /* threads_.emplace_back(std::move(ptr)); */
        std::cout << ">>>create new thread..." << std::endl;

        /* ����unique_ptr����Thread���Զ��ͷ�Thread, std::std::placeholders::_1�ǰ󶨺����Ĳ���ռλ�� */
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();  // ��ȡ�߳�id
        threads_.emplace(threadId, std::move(ptr));  // ���߳���ӵ��̳߳���
        threads_[threadId]->start();  // �����̣߳����������߳�����ȥִ������
        curThreadSize_++;  // �޸��߳�������صı���
        idleThreadSize_++;  // �½����߳��ǿ����̣߳���Ҫ+1
    }

    /* 5.���������Result���� */
    return Result(sp); // ������ֵ��Ӧ��ƥ����ֵ���õĹ��캯��
}

/* �����̳߳أ������̳߳���Ĭ�ϳ�ʼ��Ĭ���߳������� */
void ThreadPool::start(int initThreadSize)
{
    /* �����̳߳�Ϊ����״̬ */
    isPoolRunning_ = true;

    /* ��¼��ʼ�̸߳��� */
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;

    /* �����̶߳��� */
    for (int i = 0; i < initThreadSize_; i++)
    {
        /*
         * ����thread�̶߳����ʱ��ʹ�ð������̺߳�������thread�̶߳���
         * ʹ�ô����̶߳���ʱ�Զ�ִ��threadFunc�̺߳���
         * ʹ�ð���ʵ�֣����̶߳���󶨵��̳߳���
         * this��ʾ����ThreadPool,������Thread
         */
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)); // ����unique_ptr����Thread���Զ��ͷ�Thread
        int threadId = ptr->getId();  // ��ȡ�߳�id
        threads_.emplace(threadId, std::move(ptr));  // ���߳���ӵ��̳߳���

        /* threads_.emplace(std::move(ptr)); // ��ʹ��std::move��Դת�ƻᵼ�������ײ���ÿ�������ȥ����unique_ptr���±��� */
    }

    /* ���������߳�std::vector<std::unique_ptr<Thread>> threads_ */
    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start(); // ��Ҫִ��һ���̺߳������ڴ�������ʱ���а�
        idleThreadSize_++;    // ÿ����һ���̣߳���ʼ�Ŀ��е��߳�������+1
    }
}

/* �����̺߳��� �̳߳�����������̴߳���������������������̺߳������أ���Ӧ���߳�Ҳ�ͽ����� */
void ThreadPool::threadFunc(int threadId)
{
    auto lastTime = std::chrono::high_resolution_clock().now();  // ��ȡ�̸߳տ�ʼִ�������ʱ�䣬�߾���

    /* �����������ִ����ɣ��̳߳زſ��Ի��������߳���Դ */
    for (;;)
    {
        std::shared_ptr<Task> task; // ���ڴ��������������ȡ����������е����񣬻���ָ��
        /* һ�������� */
        {
            /* 1.�Ȼ�ȡ�� */
            std::unique_lock<std::mutex> lock(taskQueMtx_);  // threadpool��������ʱ��Ҳ���ȡ����� 

            std::cout << "tid:" << std::this_thread::get_id()
                << "���Ի�ȡ����..." << std::endl;
            /* 2.�ȴ�notEmpty_���� */
            /*
             * taskQue_size() == 0˵���������Ϊ�գ�lambda���ʽ����false����������
             * taskQue_size() > 0 ʱ�����������ɹ�����
             */
            
            /*
             * Cachedģʽ�£��п����Ѿ������˺ܶ���̣߳����ǿ���ʱ�䳬��60sӦ�ðѶ�����߳̽������յ�
             * �������յ�������initThreadSize_�������߳�Ҫ���л��գ�
             * ��ǰʱ�� - ��һ���߳�ִ�е�ʱ�� > 60s Ҳ��Ҫ����ʱ��
             */
            
            /* ÿһ�뷵��һ�Σ���ô���ֳ�ʱ���ػ����������ִ�з��� */
            while (taskQue_.size() == 0)  // �� + ˫���жϽ������,��ȡ��ǰ�����жϣ��������Ҳ�����жϱ�֤�̰߳�ȫ
            {
                if (!isPoolRunning_)  
                {
                    /* �̳߳ؽ������ͽ����̻߳��ա� */
                    threads_.erase(threadId);
                    exitCond_.notify_all();
                    std::cout << "threadid:" << std::this_thread::get_id()
                        << "exit" << std::endl;
                    return;  // �̺߳����������߳̽���
                }

                if (poolMode_ == PoolMode::MODE_CACHED)
                {
                    /* ����������ʱ���� */
                    if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                    {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if (dur.count() >= THREAD_MAX_IDLE_TIME
                            && curThreadSize_ > initThreadSize_)  // ������Ȼ��ʱ60s���ǲ�Ҫ��ħ�����֣�ʹ��const���������滻
                        {
                            /*
                             * ��ʼ���յ�ǰ�߳������Ĺ���:
                             * ��¼�߳�������صı�����ֵ�����޸�
                             * ���̶߳�����߳��б�������ɾ��(std::vector<std::unique_ptr<Thread>> threads_;)
                             * ��ǰû�а취֪��threadFunc�̺߳���ƥ����һ��thread�̶߳���
                             * ͨ��thread_tid�߳�id�ҵ��̣߳�����ɾ��
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
                    /* �ȴ�notEmpty_���� */
                    notEmpty_.wait(lock);
                }

                /* �̳߳�Ҫ�����������߳���Դ */
                /*
                 * whileѭ��ʱ�Ѿ��ж��̳߳��Ƿ������ˣ��˳�whileѭ�����л��� 
                if (!isPoolRunning_)
                {
                    threads_.erase(threadId);
                    std::cout << "threadid:" << std::this_thread::get_id()
                        << "exit" << std::endl;
                    exitCond_.notify_all();
                    return;  // �����̺߳��������ǽ�����ǰ�߳���
                }
                 */
            }


            idleThreadSize_--;

            std::cout << "tid:" << std::this_thread::get_id()
                << "��ȡ����ɹ�." << std::endl;

            /* 3.�����������ȡ��һ�������� */
            task = taskQue_.front(); // ��ȡ��ͷԪ��
            taskQue_.pop();          // ��ȡ������������������ɾ��
            taskSize_--;

            /* �����Ȼ��ʣ�����񣬼���֪ͨ�����߳�ִ������ */
            if (taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }

            /* 4.ȡ��һ��������Ҫ����֪ͨ */
            notFull_.notify_all(); // ȡ��һ�������ʾ������в�����ʹ��notFull_����֪ͨ��������

            /* 5.�ɹ�ȡ������Ӧ���ͷ��� */
            /* �������������������Զ��ͷ��� */
        }

        /* .��ǰ�̸߳���ִ��������� */
        if (task != nullptr)
        {
            // task->run();  // ִ�����񣻰�����ķ���ֵͨ��setVal��������Result����
            task->exec();
        }

        /* �߳�ִ�������񣬿����̵߳�������+1 */
        idleThreadSize_++;

        lastTime = std::chrono::high_resolution_clock().now();  // �����߳�ִ���������ʱ��
    }

}

/* ����̳߳�pool����״̬ */
bool ThreadPool::checkRunningState() const
{
    return isPoolRunning_;
}
