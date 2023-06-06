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

/* Any���ͣ���ʾ���Խ����������ݵ����� */
class Any
{
public:
    /* ʹ��Ĭ�Ϲ��캯���ܹ����ٴ��� */
    Any() = default;

    /* ʹ��Ĭ�����������ܹ����ٴ��� */
    ~Any() = default;

    /* ��Ϊʹ����unique_ptr������ֵ���õĿ��������븳ֵ���ڴ˽��� */
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    /* ��Ϊʹ����unique_ptr��������ֵ���õĸ�ֵ���ڴ�Ĭ��ʹ�� */
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    // ������캯��������Any���ͽ�����������������
    // ����Any����ʱ����base_����ָ��ָ���������������̬������������
    template<typename T>  // T:int    Derive<int>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {}

    // ��������ܰ�Any��������洢��data������ȡ����
    template<typename T>
    T cast_()
    {
        /* ͨ��base_�ҵ�����ָ���Derive���󣬴�����ȡdata_��Ա���� */
        /* ����ָ����Ҫת����������ָ�룬RTTI����ʶ��ǿ������ת���� */
        /* get��������������ָ���б������ָ�롣���ǵ���Щ�����Ĳ�����Ҫ���õ���ָ�룬��������ú����� */
        Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
        if (pd == nullptr)
        {
            throw "type is unmatch!";
        }
        return pd->data_;
    }
private:
    /* �������� */
    class Base
    {
    public:
        virtual ~Base() = default;  // ��������ʹ��Ĭ��ʵ��
    };


    // ����������
    /* ���������ͣ������������͵����ݣ�ͨ����̬���ʱ���������� */
    template<typename T>
    class Derive : public Base
    {
    public:
        /* ���������͵����ݱ������������У����ҷ�����̬ʱ����Ҫ��������������͵����� */
        Derive(T data) : data_(data)
        {}
        T data_;  // �������������������
    };

private:
    /*
        * ����һ������ָ�����ڶ�̬����������������
        * unique_ptr����ֵ���õĿ������캯���븳ֵɾ���ˣ���������ֵ���õĿ��������븳ֵ
    */
    std::unique_ptr<Base> base_;
};

/* ����Task�࣬Result������Ҫ��һ��Task���� */
class Task;

/* ʵ��һ���ź���ʹ�û���������������ʵ�� */
class Semaphore
{
public:
    /* ���캯��������ָ���ź�����Դ���� */
    Semaphore(int limit = 0)
        : resLimit_(limit)
    {}

    /* ʹ��Ĭ������ */
    ~Semaphore() = default;

    /* ��ȡһ���ź�����Դ */
    void wait()
    {
        std::unique_lock<std::mutex>lock(mtx_);  // ��ȡһ����
        /* �ȴ��ź�������Դ��û����Դ�������� */
        cond_.wait(lock, [&]()->bool { return resLimit_ > 0; });
        resLimit_--;
    }

    /* ����һ���ź�����Դ */
    void post()
    {
        std::unique_lock<std::mutex>lock(mtx_);  // ��ȡһ����
        resLimit_++;
        cond_.notify_all();  // ��֪�����߳��ź���+1
    }


private:
    int resLimit_;  // ��Դ����
    std::mutex mtx_;  // ������
    std::condition_variable cond_;  // �������� 
};

/* ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ����Result�������ִ����ɱ��ύ������̵߳ķ���ֵ */
class Result
{
public:
    /* ���캯����Ĭ�����������ύ��ִ�У�����ֵ��Ч */
    Result(std::shared_ptr<Task> task, bool isValid = true);

    /* ����������ʹ��ֻ��ָ�룬Ĭ���������� */
    ~Result() = default;

    /* ����һ��setVal��������ȡ����ִ����ķ���ֵ */
    void setVal(Any any);

    /* ��������û��������������ȡtask�ķ���ֵ */
    Any get();

private:
    Any any_;  // �洢����ķ���ֵ
    Semaphore sem_;  // �߳�ͨ�ŵ��ź�������֪�����Ƿ�ִ����   
    std::shared_ptr<Task> task_;  // ָ���Ӧ��ȡ����ֵ���������
    std::atomic_bool isValid_;  // �������ķ���ֵ�Ƿ���Ч
};


/* ���������� */
class Task
{
public:
    /* ���캯�� */
    Task();

    /* Ĭ���������� */
    ~Task() = default;

    /* ����setResult���� */
    void exec();

    /* �������񷵻�ֵ����run�����л�� */
    void setResult(Result* res);

    /* �û������Զ��������������ͣ���Task����̳У���дrun������ʵ���Զ���������C++��̬�� */
    virtual Any run() = 0;

private:
    Result* result_;  // �˴�����ʹ������ָ�룬�����������ָ����໥���ã�������Դ��Զ�ò����ͷţ�Result������������ڱ�Task��
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
/*
* example:
    ThreadPool pool;
    pool.start(4);
    class MyTask : public Task
    {
    public:
        void run(){ // �̴߳��� }
    };
    pool.submitTask(std::make_shared<MyTask>());
*/
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
    Result submitTask(std::shared_ptr<Task> sp);

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
