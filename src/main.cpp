#include "threadpool.h"
#include <iostream>
#include <chrono>
#include <thread>

using uLong = unsigned long long;
using std::cout;
using std::endl;
/*
	* ��Щ����ϣ������̷߳���ֵ
	* ����������1 + ... + 30000�ĺ�
		thread1����1�ӵ�10000
		thread2����10001�ӵ�20000
		thread3����20001�ӵ�30000
		main thread���̸߳�ÿһ���̷߳����������䲢�ȴ����Ǽ��㲢���صĽ�����ϲ����յĽ��

*/

/* �̳�Task,�����ύ�������߳�ִ��run */
class MyTask : public Task
{
public:
	MyTask(int begin, int end) : begin_(begin), end_(end)
	{ }

	/* ��ô���run��������ֵ���Է�����������
	 * ����Java��Python��Object������ʾ������ĸ��࣬���ֻ��Ҫģ��Objectָ������
	 * C++17�ṩ��Any����
	 */
	Any run() 
	{
		std::cout << "tid:" << std::this_thread::get_id()
			<< "begin!" << std::endl;
		// std::this_thread::sleep_for(std::chrono::seconds(3));  // �ȴ�ʱ��
		uLong sum = 0;
		for (uLong i = begin_; i < end_; i++)
		{
			sum += i;
		}

		std::cout << "tid:" << std::this_thread::get_id()
			<< "end!" << std::endl;
		return sum;  // ����һ��Any����
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
		/* ��linux�ϣ�Result����Ҳ�Ǿֲ����󣬻����� */
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		
		/* 
		 * get���������߳�δִ��������������
		 * ���������get����ôpool�ͻ��������
		 * �̳߳ض����Զ�������������������е�����δִ�����̳߳ؾͽ���
		 * ��Ҫ�����Ż�
		 */
		// uLong sum1 = res1.get().ca st_<uLong>();  // 
		// cout << sum1 << endl; 
	}   // �������Result����Ҳ����������vs�����������������ͷ������Դ

	cout << "main over" << endl;
	getchar();
#if 0
	/* ���⣺ThreadPool���������Ժ���ô�����̳߳���ص��߳���Դȫ������ */
	{
		std::cout << "hello threadpool" << std::endl;
		ThreadPool pool;

		/* �û��Զ����߳�ģʽ���ڿ���ǰ�����̳߳ع���ģʽ���̳߳ع������������ */
		pool.setMode(PoolMode::MODE_CACHED);

		/* �����̳߳� */
		pool.start(4);

		/*
			* �ύһ������ϣ�����һ��result����
			* �����ύ����һ���ܹ����̳߳ط����̴߳���
			* ��ʹ�����̴߳���Ҳδ���ܹ����̴�����
			* �����ʹ��get��ȡ����ֵʱӦ��ȷ�������Ѿ����߳�ִ����ϣ������������
			* ʹ���ź���ʵ������

		*/
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		/* ����task��ִ���꣬task����û�ˣ�������task�����ResultҲû�� */
		uLong sum1 = res1.get().cast_<uLong>();  // get������һ��Any������ôת���ɾ������ͣ�
		uLong sum2 = res2.get().cast_<uLong>();
		uLong sum3 = res3.get().cast_<uLong>();

		/*
			* Master - Slave�߳�ģ��
			* Master�����ֽ�����Ȼ�������Slave�̷߳�������
			* �ȴ�����Slave�߳�ִ�������񣬷��ؽ��
			* Master�̺߳ϲ����������������
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
	pool.submitTask(std::make_shared<MyTask>());  // ������ָ����Ϊ��������ύ����ô������д�ŵľ�������ָ�룬������̬
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
 * ��Ŀ�����������⣺�������⣨���������
 * 


 */