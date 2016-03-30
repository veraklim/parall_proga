#include <queue>
#include <thread>
#include <cstdlib>
#include <mutex>
#include <iostream>
#include <future>
#include <ctime>
#include <chrono>

using namespace std;

template <typename T>
class thread_safe_queue{
public:

	thread_safe_queue() : capacity(0), flag_shutdown(false){}

	thread_safe_queue(size_t cap){
		capacity = cap;
		flag_shutdown = false;
	}

	thread_safe_queue(const thread_safe_queue&) = delete;

	void enqueue(T item){
		unique_lock<mutex> mtx(_mtx);
		while (thr_queue.size() == capacity) {
			not_full_cv.wait(mtx);
		}
		thr_queue.push(item);
		not_empty_cv.notify_all();
	}

	void pop(T& item) {
		unique_lock<mutex> mtx(_mtx);
		while (thr_queue.empty()) {
			//останавливает потребителя	если flag_shutdown==true
			if (flag_shutdown)	{
				return;
			}
			not_empty_cv.wait(mtx);
		}
		item = thr_queue.front();
		thr_queue.pop();
		not_full_cv.notify_all();
	}

	void shutdown(){
		flag_shutdown = true;
	}

private:
	queue<T> thr_queue;
	size_t capacity;
	mutex _mtx;
	condition_variable not_empty_cv;
	condition_variable not_full_cv;
	bool flag_shutdown;
};


