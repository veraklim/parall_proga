#include<memory>
#include<future>
#include<thread>
#include <mutex>
#include <exception>
#include<vector>
#include<iostream>
#include<chrono>
#include<cstdlib>
#include<queue>
#include<ctime>


using namespace std;

template <typename T, class Container = deque<T>>
class thread_safe_queue{
public:

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

	bool pop(T& item) {
		unique_lock<mutex> mtx(_mtx);
		while (thr_queue.empty()) {
			if (flag_shutdown)
				return false;
			not_empty_cv.wait(mtx);
		}
		item = thr_queue.front();
		thr_queue.pop();
		not_full_cv.notify_all();
		return true;
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



template<typename T>
class thread_pool{
public:
	thread_pool();
	explicit thread_pool(size_t num_work);

	//отправить задачу в пул
	shared_ptr<future<T>> submit(function<T()> func_perform);

	void shutdown(){
		flag_shutdown.store(true);
		task_queue.shutdown();
	};

	~thread_pool();
private:
	vector<thread> workers;
	size_t num_workers;
	size_t default_num_workers();
	atomic_bool flag_shutdown;

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
thread_pool<T>::thread_pool() :thread_pool(default_num_workers()){}

template<typename T>
thread_pool<T>::thread_pool(size_t num_work) : task_queue(100), num_workers(num_work) {
	flag_shutdown.store(false);
	//запускаем рабочие потоки
	for (size_t i = 0; i < num_workers; i++)
		workers.emplace_back(thread([this]() -> void {
		//берём задание
		Task cur_task;
		while (true){
			try{
				if (task_queue.pop(cur_task)) {
					cur_task.promise_ptr->set_value(cur_task.job());
				}
				else{
					return;
				}
			}
			catch (exception&){
				cur_task.promise_ptr->set_exception(current_exception());
			};
		}
	}));
}

template<typename T>
shared_ptr<future<T>> thread_pool<T>::submit(function<T()> func_perform){
	auto my_promise_ptr = make_shared<promise<T>>(promise<T>());
	//добавляем задачу в очередь
	task_queue.enqueue(Task(func_perform, my_promise_ptr));
	return make_shared<future<T>>(my_promise_ptr->get_future());
}

template<typename T>
size_t thread_pool<T>::default_num_workers(){
	size_t n = thread::hardware_concurrency();
	return (n == 0) ? n : 4;
}

template<typename T>
thread_pool<T>::~thread_pool(){
	if (!flag_shutdown){
		shutdown();
	}
	for (auto& thread : workers)
		if (thread.joinable()) thread.join();
}

