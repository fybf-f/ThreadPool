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
		[&]()->bool { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		// ��ʾnut_Full_�ȴ�1s�ӣ�������Ȼû������
		std::cerr << "task queue is full, submit task fail." << std::endl;
		return;
	}

	/* 3.����п��࣬������ŵ���������� */
	/* 
		* ���������ŵ���������У��������������Ļ�����һ��wait�ͷ�����
		* �����������̳߳��е��߳���ռ������������
		* ���ٽ���taskQue_�Ĳ�������Ϊû����������������ֱ�������߳��ͷ��������������Ϊ��
	*/
	taskQue_.emplace(sp);  
	taskSize_++;  // std::atomic_int taskSize_; ԭ������

	/* 4.��Ϊ�·�������������п϶����գ���notEmpty_�Ͻ���֪ͨ */
	notEmpty_.notify_all();  // ����������������в��գ�֪ͨ�̳߳ط����߳�ִ������


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
		threads_.emplace_back(std::move(ptr));  // ��ʹ��std::move��Դת�ƻᵼ�������ײ���ÿ�������ȥ����unique_ptr���±���  
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
	/*
	std::cout << "begin threadFunc tid:" << std::this_thread::get_id() 
			  <<std::endl;
	std::cout << "end threadFunc tid:" << std::this_thread::get_id() 
		      <<std::endl;
	*/
	for (;;)
	{
		std::shared_ptr<Task> task;  // ���ڴ��������������ȡ����������е�����
		/* һ�������� */ 
		{
			/* 1.�Ȼ�ȡ�� */
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			/* 2.�ȴ�notEmpty_���� */
			/*
				* taskQue_size() == 0˵���������Ϊ�գ�lambda���ʽ����false����������
				* taskQue_size() > 0 ʱ�����������ɹ�����
			*/
			notEmpty_.wait(lock, [&]()->bool { return taskQue_.size() > 0; });
			/* 3.�����������ȡ��һ�������� */
			task = taskQue_.front();  // ��ȡ��ͷԪ��
			taskQue_.pop();  // ��ȡ������������������ɾ��
			taskSize_--;

			/* �����Ȼ��ʣ�����񣬼���֪ͨ�����߳�ִ������ */
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			/* 4.ȡ��һ��������Ҫ����֪ͨ */
			notFull_.notify_all();  // ȡ��һ�������ʾ������в�����ʹ��notFull_����֪ͨ��������

			/* 5.�ɹ�ȡ������Ӧ���ͷ��� */
			/* �������������������Զ��ͷ��� */
		}
	
		/* .��ǰ�̸߳���ִ��������� */
		
		if (task != nullptr)
		{
			task->run();
		
		}
		
	}
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
