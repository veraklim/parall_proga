#include <vector>
#include <iostream>
#include <forward_list>
#include <mutex>

using namespace std;

class rw_mutex : public mutex {
public:
	rw_mutex() : readers(0) {}

	//блок reader/writer
	void read_lock(){
		turnstile.lock();
		turnstile.unlock();
		lightswitch.lock();
		readers += 1;
		if (readers == 1)
			ring.lock();
		lightswitch.unlock();
	}
	void write_lock() {
		turnstile.lock();
		ring.lock();
		turnstile.unlock();
	}
	void read_unlock(){
		lightswitch.lock();
		readers -= 1;
		if (readers == 0)
			ring.unlock();
		lightswitch.unlock();
	}
	void write_unlock() {
		ring.unlock();
	}
private:
	mutex ring;
	mutex turnstile;
	mutex lightswitch;
	int readers;
};

template <typename T, class H = hash<T> >
class striped_hash_set {
public:
	striped_hash_set(size_t num_stripes = 1, int growth_factor = 2, int load_factor = 5);

	void add(const T& e);
	void remove(const T& e);
	bool contains(const T& e);

private:
	//расширение 
	void rehash();

	size_t _num_stripes;  //число потоков
	int _growth_factor;	  //коэффициент увеличения
	int _load_factor; // коэффициент заполняемости

	size_t get_stripe_index(size_t hash_value);
	size_t get_bucket_index(size_t hash_value);

	//текущие параметры таблицы
	size_t num_of_elem;
	size_t cur_table_size;
	double cur_load_factor(){ return num_of_elem / cur_table_size; };
	mutex size_change_mtx;

	vector<forward_list<T>> table;
	vector<rw_mutex> mtx;
};

template <typename T, class H >
striped_hash_set<T, H>::striped_hash_set(size_t num_stripes, int growth_factor, int load_factor)
	:_num_stripes(num_stripes), _growth_factor(growth_factor), _load_factor(load_factor), num_of_elem(0), cur_table_size(2 * num_stripes), table(cur_table_size), mtx(num_stripes){}

template <typename T, class H >
inline void striped_hash_set<T, H>::add(const T& e){
	if (cur_load_factor() >= _load_factor) {
		rehash();
	}

	size_t key = H()(e);
	mtx[key%_num_stripes].write_lock();
	int i = key% cur_table_size;
	auto x = find(table[i].begin(), table[i].end(), e);
	if (x == table[i].end()) {
		table[i].push_front(e);
	}

	mtx[key%_num_stripes].write_unlock();

	size_change_mtx.lock();
	num_of_elem++;
	size_change_mtx.unlock();

}

template <typename T, class H>
inline void striped_hash_set<T, H>::remove(const T& e){
	size_t key = H()(e);
	mtx[key%_num_stripes].write_lock();
	size_t i = key% cur_table_size;
	table[i].remove(e);
	mtx[key%_num_stripes].write_unlock();

	size_change_mtx.lock();
	num_of_elem--;
	size_change_mtx.unlock();
}

template <typename T, class H >
inline bool striped_hash_set<T, H>::contains(const T& e){
	size_t key = H()(e);
	mtx[key%_num_stripes].read_lock();
	size_t i = key% cur_table_size;
	auto x = find(table[i].begin(), table[i].end(), e);
	mtx[key%_num_stripes].read_unlock();
	if (x == table[i].end()) {
		return 0;
	}
	return 1;
}

template <typename T, class H >
inline void striped_hash_set<T, H>::rehash(){
	size_t prev_size = cur_table_size;
	mtx[0].write_lock();
	if (prev_size != cur_table_size) {
		return;
	}

	for (size_t i = 1; i < _num_stripes; i++) {
		mtx[i].write_lock();
	}
	cur_table_size *= _growth_factor;

	vector<forward_list<T>> new_table(cur_table_size);
	for (size_t i = 0; i < prev_size; i++) {
		for (auto j = table[i].begin(); j != table[i].end(); ++j) {
			new_table[H()(*j) % cur_table_size].push_front(*j);
		}
	}
	swap(table, new_table);

	for (size_t i = 0; i < _num_stripes; i++) {
		mtx[i].write_unlock();
	}
}
