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

	thread_safe_queue(size_t cap){
		capacity = cap;
		flag_shutdown = false;
	}

	thread_safe_queue(const thread_safe_queue&) = delete;

	void enqueue(T item){
		unique_lock<mutex> mtx(_mtx);
		while (thr_queue.size()==capacity) {
			not_full_cv.wait(mtx);
		}
		thr_queue.push(item);
		not_empty_cv.notify_all();
	}
	
	void pop(T& item) {
		unique_lock<mutex> mtx(_mtx);
		while (thr_queue.empty()) {
			//îñòàíàâëèâàåò ïîòðåáèòåëÿ	åñëè flag_shutdown==true
			if (flag_shutdown)
				return;
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


void produce(thread_safe_queue<int>& tasks){
	for (int i = 1; i < 5600; i++) {
		tasks.enqueue(i);
	}
	tasks.shutdown();
}

void consume(thread_safe_queue<int>& tasks, mutex& writemtx, int num){
	srand((unsigned int)time(NULL));
	while (true) {
		int mod;
		tasks.pop(mod);
		writemtx.lock();
		cout <<mod <<" "<< (mod^rand()) << " " << num << "\n";	 //âîçâðàùàåò â ñòåïåíü ñëó÷àéíîãî ÷èñëà
		writemtx.unlock();
	}
}
int main() {
	thread_safe_queue<int> tasks(100);
	mutex writemtx;
	thread producer(produce, ref(tasks));
	vector<thread> consumers;
	for (int i = 0; i < 10; i++) {
		consumers.emplace_back(consume, ref(tasks), ref(writemtx), i);
	}
	for (int i = 0; i < 10; i++) {
		consumers[i].join();
	}
	producer.join();
	return 0;
}
