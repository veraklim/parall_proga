
#include "thread_safe_queue.h"
#include<memory>
#include<future>
#include<thread>
#include<vector>
#include<iostream>
#include<cstdlib>
#include<ctime>


using namespace std;

template<typename T>
class ThreadPool{
public:
	ThreadPool();
	ThreadPool(size_t num_work);

	//отправить задачу в пул
	shared_ptr<future<T>> submit(function<T()> func_perform);

	void shut_down(){ task_queue.shutdown(); };

	~ThreadPool();
private:
	vector<thread> workers;
	size_t num_workers;
	size_t default_num_workers();

	struct Task{
		function<T()> job;
		shared_ptr<promise<T>> promise_ptr;

		Task() : job(nullptr), promise_ptr(nullptr){}
		Task(function<T()> j, shared_ptr<promise<T>> p) : job(j), promise_ptr(p){}
	};

	//потокобезопасная очередь заданий
	thread_safe_queue<Task> task_queue;
};

template<typename T>
ThreadPool<T>::ThreadPool() :ThreadPool(default_num_workers()){}

template<typename T>
ThreadPool<T>::ThreadPool(size_t num_work) : task_queue(100), num_workers(num_work) {
	//запускаем рабочие потоки
	for (size_t i = 0; i < num_workers; i++)
		workers.push_back(thread([this]() -> void {
		//берём задание
		Task cur_task;
		task_queue.pop(cur_task);
		while (cur_task.job != nullptr) {
			Task x = cur_task;
			cur_task.promise_ptr->set_value(cur_task.job());
			task_queue.pop(cur_task);
		}
	}));
}

template<typename T>
shared_ptr<future<T>> ThreadPool<T>::submit(function<T()> func_perform){
	auto my_promise_ptr = make_shared<promise<T>>(promise<T>());
	//добавляем задачу в очередь
	task_queue.enqueue(Task(func_perform, my_promise_ptr));
	return make_shared<future<T>>(my_promise_ptr->get_future());
}

template<typename T>
size_t ThreadPool<T>::default_num_workers(){
	size_t n = thread::hardware_concurrency();
	return (n == 0) ? n : 4;
}

template<typename T>
ThreadPool<T>::~ThreadPool(){
	shut_down();

	for (auto& thread : workers)
		if (thread.joinable()) thread.join();
}


using namespace std;
bool f(int n){
	if (n%2 == 0)
		return true;
	return false;
}

int main(){
	vector<int> values;
	vector<shared_ptr<future<bool>>> res;
	ThreadPool<bool> pool;
	srand(time(0));

	for (int i = 0; i < 1000; i++){
		values.push_back(rand());
		res.push_back(pool.submit(bind(f, values[i])));
	}

	for (int i = 0; i < 1000; i++){
		cout << values[i] << (res[i]->get() ? " is even" : " is not even")<< endl ;
	}

	return 0;
}	 