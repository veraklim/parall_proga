#include <memory>
#include <atomic>

using namespace std;

template <typename T>
class lock_free_queue
{
public:
	lock_free_queue() : head(new node), tail(head.load()), to_delete(head.load()) {}
	void enqueue(T item);
	bool dequeue(T& item);
	~lock_free_queue(){
		for (node* i = to_delete.load(); i != nullptr;){
			node* next = i->next.load();
			delete i;
			i = next;
		}
	}
private:
	struct node
	{
		node() : next(nullptr) {}
		shared_ptr<T> value;
		atomic<node*> next;
		node(T& item) : value(new T(item)), next(nullptr) {}
	};

	atomic<node*> head;
	atomic<node*> tail;
	atomic<size_t> threads_in_dequeue; //количество потоков, которые пытаются извлечь что-то из очереди 
	atomic<node*> to_delete; //узлы, которые надо удалить из памяти
};

template<class T>
void lock_free_queue<T>::enqueue(T item)
{
	node* new_node = new node(item);
	node* cur_tail;
	while (true)
	{
		cur_tail = tail.load();
		node* cur_tail_next = cur_tail->next;

		if (!cur_tail_next)
		{
			if (tail.load()->next.compare_exchange_weak(cur_tail_next, new_node)) //цепляет новый узел
				break;
		}
		else
			tail.compare_exchange_weak(cur_tail, cur_tail_next); //helping
	}
	tail.compare_exchange_weak(cur_tail, new_node); //переставляет указатель tail
}

template<typename T>
bool lock_free_queue<T>::dequeue(T& item)
{
	node* cur_head_next;
	threads_in_dequeue.fetch_add(1);  

	while (true)
	{
		node* cur_head = head.load();
		node* cur_tail = tail.load();
		cur_head_next = cur_head->next;
		if (cur_head == cur_tail)
		{
			if (!cur_head_next)
				return false;
			else
				tail.compare_exchange_weak(cur_head, cur_head_next); // helping
		}
		else
		{
			if (head.compare_exchange_weak(cur_head, cur_head_next))
			{
				item = *cur_head_next->value;
				return true;
			}
		}
	}

	// Уничтожаем все узлы, которые были извлечены из очереди
	if (threads_in_dequeue.load() == 1)
	{
		node* nodes_to_delete = to_delete.exchange(cur_head_next); // список подлежащих удалению узлов
		while (nodes_to_delete != to_delete.load()) //обходим список и удаляем узлы
		{
			node* swp = nodes_to_delete->next.load();
			delete nodes_to_delete;
			nodes_to_delete = swp;
		}
	}
	threads_in_dequeue.fetch_sub(1);

	return true;
}
