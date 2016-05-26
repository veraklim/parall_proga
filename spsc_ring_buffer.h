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
	size_t cur_tail = tail.load(memory_order_relaxed); //� tail �������� ������ producer, ����� ������ relaxed
	size_t cur_head = head.load(memory_order_acquire);	//���� ����� � consumer � producer ������ - acquire

	//��������� ����� ����� ������� � ������� ������� � ������ ���� �� ���� ��������� ���� (����� ��������� ������ � ������ �������)
	if ((cur_tail + 1) % capacity == cur_head){
		return false;
	}
	data[cur_tail] = move(e);
	tail.store((cur_tail + 1) % capacity, memory_order_release); //��� ������ ���� ����� � consumer � producer ������ ���������
	return true;
}


template <typename T>
inline bool spsc_ring_buffer<T>::dequeue(T& e)
{
	//���������� enqueue �������� memory ordering
	size_t cur_head = head.load(memory_order_relaxed);
	size_t cur_tail = tail.load(memory_order_acquire);

	if (cur_head == cur_tail) {
		return false;
	}
	e = data[cur_head];
	head.store((cur_head + 1) % capacity, memory_order_release);
	return true;
}
