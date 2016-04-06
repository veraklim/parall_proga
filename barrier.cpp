#include <mutex>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <thread>

using namespace std;

class Barrier {
public:
	Barrier(size_t num_thr) :num_threads(num_thr), coming_threads(0), inside_threads(0){};

	void enter();

private:
	size_t num_threads;
	//первые ворота - на вход
	condition_variable cv_first;

	//вторые ворота - на выход
	condition_variable cv_second;
	
	mutex mtx;

	//число потоков, которые хотят пройти через барьер
	size_t coming_threads;

	//число потоков, находящихся внутри барьера (перед 2ми воротами)
	size_t inside_threads;
};

void Barrier::enter(){
	unique_lock<mutex> lock(mtx);

	//пока кто-то есть внутри, заходить нельзя
	while (inside_threads) {
		cv_first.wait(lock);
	}
	coming_threads++;

	//пришли все
	if (coming_threads == num_threads) {
		cv_second.notify_all();
		inside_threads = num_threads;
	}

	//ждем остальных
	while (coming_threads < num_threads) {
		cv_second.wait(lock);
	}
	inside_threads--;

	//если все вышли, первые ворота можно открыть
	if (!inside_threads) {
		coming_threads = 0;
		cv_first.notify_all();
	}
}

int main() {
	vector< thread> threads;
	Barrier barrier(10);
	for (int i = 0; i < 10; i++) {
		threads.push_back( thread([&barrier]() -> void {
			int j = 0;
			while (j < 100) {
				 cout << j;
				j++;
				barrier.enter();
			}
		}));
	}
	for (int i = 0; i < 10; i++) {
		threads[i].join();
	}
}
