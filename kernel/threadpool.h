#include<iostream>
#include<thread>
#include<vector>
#include<queue>
#include<stdexcept>
#include<condition_variable>
#include<memory>
#include<assert.h>
using namespace std;
const int MAX_THREADS = 1000;
//�̰߳�ȫ����
template<typename T>
class ThreadSafeQue {
public:
	bool empty() {
		unique_lock<mutex>lk(queue_mutex);
		return _queue.empty;
	}
	bool pop(T &ret) {
		unique_lock<mutex>lk(queue_mutex);
		if (_queue.empty()) {
			return false;
		}
		ret=_queue.front();
		_queue.pop();
		return true;
	}
	void push(T &data) {
		unique_lock<mutex>lk(queue_mutex);
		_queue.push(data);
		lk.unlock();
		condition.notify_all();
	}
	ThreadSafeQue(){};
	~ThreadSafeQue() {};
	mutex queue_mutex;
private:
	condition_variable condition;
	queue<T>_queue;
};
template<typename T>
class threadPool {
public:
	threadPool(int number = 1);
	~threadPool();
	bool appand(T* request);//���������
private:
	mutex pool_mutex;//�̳߳ص���
	bool stop;//ֹͣ��־
	
	static void* worker(void* arg);//ÿ���̶߳�Ҫִ�еĺ���
	void run();

	ThreadSafeQue<T*>task_queue;//������У��ṩ����
	vector<thread>work_threads;//��Ŷ�������߳�
};
template<typename T>
threadPool<T>::threadPool(int number) :stop(false) {
	//����С�����������ֵ
	if (number <= 0 || number > MAX_THREADS) {
		throw exception();
	}
	//���������̴߳浽�߳�1��
	cout << "threadPool create " << number << " threads" << endl;
	for (int i = 0; i < number; ++i) {
		unique_lock<mutex> lk(pool_mutex);
		work_threads.emplace_back(worker, this);
	}
}
template<typename T>
threadPool<T>::~threadPool() {
	//��stop������Ҫ����
	unique_lock<mutex> lk(pool_mutex);
	stop = true;
	//�ȴ������߳̽���
	for (auto& wt : work_threads) {
		wt.join();
	}
	cout << "ThreadPool is already closed" << endl;
}
template<typename T>
bool threadPool<T>::appand(T* request) {
	task_queue.push(request);
	return true;
}
template<typename T>
void* threadPool<T>::worker(void* arg) {
	//�߳�����
	threadPool* pool = (threadPool*)arg;
	pool->run();
	return pool;
}
template<typename T>
void threadPool<T>::run() {
	//��δֹͣ�����ȡ����
	while (!stop) {
		T* request;
		if (this->task_queue.pop(request)) {
			cout << "thread" << this_thread::get_id << ":" << endl;
			request->process();
		}
	}
}
class Request {
public:
	virtual void process() = 0;
};
