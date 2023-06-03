#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = 1024;

/* 线程池构造函数(使用初始化参数列表) */
ThreadPool::ThreadPool()
	: initThreadSize_(0)  // 初始线程数量,一般使用fixed模式需要获取cpu核心数量
	, taskSize_(0)  // 任务数量
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)  // 任务数量上限阈值
	, poolMode_(PoolMode::MODE_FIXED)  // 线程池工作模式，默认使用Fixed工作模式
{}

/* 线程池析构 */
ThreadPool::~ThreadPool()
{}

/* 设置线程池的工作模式 */
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

/* 设置task任务队列上限阈值 */
void ThreadPool::setTaskQueMaxThreshHold(int threshHold)
{
	taskQueMaxThreshHold_ = threshHold;
}

/* 给线程池提交任务（重难点）用户调用该接口，传入任务对象，生产任务 */
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	/* 1.获取锁 */
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	/* 2.线程的通信 等待任务队列有空余 */
	/* 
		* 保证任务队列不满，如果满就在此进入阻塞
		* std::condition_variable notFull_;  // 表示任务队列不满
		* notFull_.wait();第一个参数传入智能锁(unique_lock)，第二个参数传入函数，这里使用lambda表达式，并使用值捕获
		* 用户提交任务，最长时间不能阻塞超过1s，否则判断任务提交失败，返回
	*/
	notFull_.wait(lock, [&]()->bool { return taskQue_.size() > taskQueMaxThreshHold_;  });  

	/* 3.如果有空余，把任务放到任务队列中 */
	taskQue_.emplace(sp);  // 将任务对象放到任务队列中
	taskSize_++;  // std::atomic_int taskSize_; 原子类型

	/* 4.因为新放了任务，任务队列肯定不空，在notEmpty_上进行通知 */
	notEmpty_.notify_all();  // 任务队列不空，通知线程池分配线程执行任务


}

/* 开启线程池（开启线程池是默认初始化默认线程数量） */
void ThreadPool::start(int initThreadSize)
{
	/* 记录初始线程个数 */
	initThreadSize_ = initThreadSize;

	/* 创建线程对象 */
	for (int i = 0; i < initThreadSize_; i++)
	{
		/*
			* 创建thread线程对象的时候，使用绑定器把线程函数给到thread线程对象
			* 使得创建线程对象时自动执行threadFunc线程函数
			* 使用绑定器实现，将线程对象绑定到线程池上
			* this表示的是ThreadPool,而不是Thread
		*/
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));  // 创建unique_ptr管理Thread，自动释放Thread
		threads_.emplace_back(std::move(ptr));  // 不使用std::move资源转移会导致unique_ptr调用拷贝构造报错  
	}

	/* 启动所有线程std::vector<std::unique_ptr<Thread>> threads_ */
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();  // 需要执行一个线程函数，在创建对象时进行绑定
	}
}

/* 定义线程函数 线程池里面的所有线程从任务队列里面消费任务 */
void ThreadPool::threadFunc() 
{
	std::cout << "begin threadFunc tid:" << std::this_thread::get_id() 
			  <<std::endl;
	std::cout << "end threadFunc tid:" << std::this_thread::get_id() 
		      <<std::endl;
}

/* ----------------- 线程方法实现 ----------------- */

/* 线程构造 */
Thread::Thread(ThreadFunc func)
	: func_(func)  // 使用func_接收绑定器绑定的线程函数对象
{

}


/* 线程析构 */
Thread::~Thread()
{

}

/* 启动线程 */
void Thread::start()
{
	/* 创建一个线程来执行一个线程函数 */
	std::thread t(func_);  // C++11来说 线程对象t  和线程函数func
	/* 
		* 设置分离线程，将线程对象与线程函数分离，防止出现孤儿进程 
		* Linux中使用pthread_detach pthread_t设置分离线程
	*/
	t.detach();
}
