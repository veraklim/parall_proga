#include <iostream>
#include <atomic>
#include <vector>

using namespace std;

template <class T>
class spsc_ring_buffer {
public:
	explicit spsc_ring_buffer(size_t size) :data(size + 1), tail(0), head(0), capacity(size + 1) {}
	bool enqueue(T e);
	bool dequeue(T& e);
private:
	vector<T> data;
	atomic<size_t> tail;
	atomic<size_t> head;
	const size_t capacity;
};


template <typename T>
inline bool spsc_ring_buffer<T>::enqueue(T e)
{
	size_t cur_tail = tail.load(memory_order_relaxed); //с tail работает только producer, можно читать relaxed
	size_t cur_head = head.load(memory_order_acquire);	//надо чтобы и consumer и producer читали - acquire

	//оставляем чтобы между хвостом и головой очереди в буфере хотя бы один свободный слот (чтобы различать пустую и полную очереди)
	if ((cur_tail + 1) % capacity == cur_head){
		return false;
	}
	data[cur_tail] = move(e);
	tail.store((cur_tail + 1) % capacity, memory_order_release); //для записи надо чтобы и consumer и producer видели изменения
	return true;
}


template <typename T>
inline bool spsc_ring_buffer<T>::dequeue(T& e)
{
	//аналогично enqueue выбираем memory ordering
	size_t cur_head = head.load(memory_order_relaxed);
	size_t cur_tail = tail.load(memory_order_acquire);

	if (cur_head == cur_tail) {
		return false;
	}
	e = data[cur_head];
	head.store((cur_head + 1) % capacity, memory_order_release);
	return true;
}
