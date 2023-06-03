#include "threadpool.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
	std::cout << "hello threadpool" << std::endl;
	ThreadPool pool;
	pool.start(6);
	/*  */
	std::this_thread::sleep_for(std::chrono::seconds(5));
	return 0;
}

