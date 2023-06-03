#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = 1024;

/* �̳߳ع��캯��(ʹ�ó�ʼ�������б�) */
ThreadPool::ThreadPool()
	: initThreadSize_(0)  // ��ʼ�߳�����,һ��ʹ��fixedģʽ��Ҫ��ȡcpu��������
	, taskSize_(0)  // ��������
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)  // ��������������ֵ
	, poolMode_(PoolMode::MODE_FIXED)  // �̳߳ع���ģʽ��Ĭ��ʹ��Fixed����ģʽ
{}

/* �̳߳����� */
ThreadPool::~ThreadPool()
{}

/* �����̳߳صĹ���ģʽ */
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

/* ����task�������������ֵ */
void ThreadPool::setTaskQueMaxThreshHold(int threshHold)
{
	taskQueMaxThreshHold_ = threshHold;
}

/* ���̳߳��ύ�������ѵ㣩�û����øýӿڣ�������������������� */
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	/* 1.��ȡ�� */
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	/* 2.�̵߳�ͨ�� �ȴ���������п��� */
	/* 
		* ��֤������в�������������ڴ˽�������
		* std::condition_variable notFull_;  // ��ʾ������в���
		* notFull_.wait();��һ����������������(unique_lock)���ڶ����������뺯��������ʹ��lambda���ʽ����ʹ��ֵ����
		* �û��ύ�����ʱ�䲻����������1s�������ж������ύʧ�ܣ�����
	*/
	notFull_.wait(lock, [&]()->bool { return taskQue_.size() > taskQueMaxThreshHold_;  });  

	/* 3.����п��࣬������ŵ���������� */
	taskQue_.emplace(sp);  // ���������ŵ����������
	taskSize_++;  // std::atomic_int taskSize_; ԭ������

	/* 4.��Ϊ�·�������������п϶����գ���notEmpty_�Ͻ���֪ͨ */
	notEmpty_.notify_all();  // ������в��գ�֪ͨ�̳߳ط����߳�ִ������


}

/* �����̳߳أ������̳߳���Ĭ�ϳ�ʼ��Ĭ���߳������� */
void ThreadPool::start(int initThreadSize)
{
	/* ��¼��ʼ�̸߳��� */
	initThreadSize_ = initThreadSize;

	/* �����̶߳��� */
	for (int i = 0; i < initThreadSize_; i++)
	{
		/*
			* ����thread�̶߳����ʱ��ʹ�ð������̺߳�������thread�̶߳���
			* ʹ�ô����̶߳���ʱ�Զ�ִ��threadFunc�̺߳���
			* ʹ�ð���ʵ�֣����̶߳���󶨵��̳߳���
			* this��ʾ����ThreadPool,������Thread
		*/
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));  // ����unique_ptr����Thread���Զ��ͷ�Thread
		threads_.emplace_back(std::move(ptr));  // ��ʹ��std::move��Դת�ƻᵼ��unique_ptr���ÿ������챨��  
	}

	/* ���������߳�std::vector<std::unique_ptr<Thread>> threads_ */
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();  // ��Ҫִ��һ���̺߳������ڴ�������ʱ���а�
	}
}

/* �����̺߳��� �̳߳�����������̴߳�������������������� */
void ThreadPool::threadFunc() 
{
	std::cout << "begin threadFunc tid:" << std::this_thread::get_id() 
			  <<std::endl;
	std::cout << "end threadFunc tid:" << std::this_thread::get_id() 
		      <<std::endl;
}

/* ----------------- �̷߳���ʵ�� ----------------- */

/* �̹߳��� */
Thread::Thread(ThreadFunc func)
	: func_(func)  // ʹ��func_���հ����󶨵��̺߳�������
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
	std::thread t(func_);  // C++11��˵ �̶߳���t  ���̺߳���func
	/* 
		* ���÷����̣߳����̶߳������̺߳������룬��ֹ���ֹ¶����� 
		* Linux��ʹ��pthread_detach pthread_t���÷����߳�
	*/
	t.detach();
}
