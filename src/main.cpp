#include "threadpool.h"
#include <iostream>
#include <chrono>
#include <thread>

using uLong = unsigned long long;
using std::cout;
using std::endl;
/*
	* 有些场景希望获得线程返回值
	* 举例：计算1 + ... + 30000的和
		thread1计算1加到10000
		thread2计算10001加到20000
		thread3计算20001加到30000
		main thread主线程给每一个线程分配计算的区间并等待他们计算并返回的结果，合并最终的结果

*/

/* 继承Task,用于提交任务并让线程执行run */
class MyTask : public Task
{
public:
	MyTask(int begin, int end) : begin_(begin), end_(end)
	{ }

	/* 怎么设计run函数返回值可以返回任意类型
	 * 例如Java，Python的Object，他表示所有类的父类，因此只需要模仿Object指向子类
	 * C++17提供了Any类型
	 */
	Any run() 
	{
		std::cout << "tid:" << std::this_thread::get_id()
			<< "begin!" << std::endl;
		// std::this_thread::sleep_for(std::chrono::seconds(3));  // 等待时间
		uLong sum = 0;
		for (uLong i = begin_; i < end_; i++)
		{
			sum += i;
		}

		std::cout << "tid:" << std::this_thread::get_id()
			<< "end!" << std::endl;
		return sum;  // 返回一个Any对象
	}
private:
	int begin_;
	int end_;
};


int main(int argc, char* argv[])
{
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);
		pool.start(2);
		/* 在linux上，Result对象也是局部对象，会析构 */
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		
		/* 
		 * get函数会在线程未执行完任务阻塞，
		 * 如果不调用get，那么pool就会出作用域
		 * 线程池对象自动析构，导致任务队列中的任务未执行完线程池就结束
		 * 需要进行优化
		 */
		// uLong sum1 = res1.get().ca st_<uLong>();  // 
		// cout << sum1 << endl; 
	}   // 出了这里，Result对象也会析构，在vs下条件变量析构会释放相关资源

	cout << "main over" << endl;
	getchar();
#if 0
	/* 问题：ThreadPool对象析构以后，怎么样把线程池相关的线程资源全部回收 */
	{
		std::cout << "hello threadpool" << std::endl;
		ThreadPool pool;

		/* 用户自定义线程模式，在开启前调整线程池工作模式，线程池工作后不允许更改 */
		pool.setMode(PoolMode::MODE_CACHED);

		/* 开启线程池 */
		pool.start(4);

		/*
			* 提交一个任务希望获得一个result对象
			* 我们提交任务不一定能够被线程池分配线程处理
			* 即使分配线程处理也未必能够立刻处理完
			* 因此在使用get获取返回值时应该确定任务已经被线程执行完毕，否则进行阻塞
			* 使用信号量实现阻塞

		*/
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		/* 随着task被执行完，task对象没了，依赖于task对象的Result也没了 */
		uLong sum1 = res1.get().cast_<uLong>();  // get返回了一个Any类型怎么转换成具体类型？
		uLong sum2 = res2.get().cast_<uLong>();
		uLong sum3 = res3.get().cast_<uLong>();

		/*
			* Master - Slave线程模型
			* Master用来分解任务，然后给各个Slave线程分配任务
			* 等待各个Slave线程执行完任务，返回结果
			* Master线程合并各个任务结果，输出
		*/

		std::cout << sum1 + sum2 + sum3 << std::endl;
	}


	/*
	uLong sum = 0;
	for (uLong i = 1; i < 300000000; i++)
	{
		sum += i;
	}
	std::cout << sum << std::endl;
	*/

	/*
	pool.submitTask(std::make_shared<MyTask>());  // 将子类指针作为任务进行提交，那么任务队列存放的就是子类指针，发生多态
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	*/

	/*  */
	getchar();
#endif
	return 0;
}



/*
 * 项目中遇到的问题：死锁问题（简历输出）
 * 


 */