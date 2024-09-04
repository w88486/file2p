#ifndef THREADPOLL_H
#define THREADPOLL_H
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <stdexcept>
#include <condition_variable>
#include <memory>
#include <assert.h>
using namespace std;
const int MAX_THREADS = 1000;
//?????????
template <typename T>
class ThreadSafeQue
{
public:
    bool empty()
    {
        pthread_mutex_lock(&queue_mutex);
        return _queue.empty();
        pthread_mutex_unlock(&queue_mutex);
    }
    bool pop(T **ret)
    {
        pthread_mutex_lock(&queue_mutex);
        // 生产者消费者模型
        while (_queue.empty())
        {
            // 消费者等待
            pthread_cond_wait(&condition, &queue_mutex);
        }
        *ret = _queue.front();
        _queue.pop();
        pthread_mutex_unlock(&queue_mutex);
        return true;
    }
    void push(T *data)
    {
        // 生产者
        pthread_mutex_lock(&queue_mutex);
        _queue.push(data);
        pthread_mutex_unlock(&queue_mutex);
        pthread_cond_signal(&condition);
    }
    ThreadSafeQue()
    {
        pthread_cond_init(&condition, NULL);
    }
    ~ThreadSafeQue()
    {
        pthread_mutex_destroy(&queue_mutex);
        pthread_cond_destroy(&condition);
    }
private:
    pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condition;
    queue<T*> _queue;
};
template <typename T>
class threadPool
{
public:
    threadPool(int number = 1);
    ~threadPool();
    bool appand(T *request); //?????????
private:
    mutex pool_mutex; //???????
    bool stop;		  //?????

    static void *worker(void *arg); //??????????��????
    void run();

    ThreadSafeQue<T> task_queue; //??????��???????
    vector<thread> work_threads;   //????????????
};
template <typename T>
threadPool<T>::threadPool(int number) : stop(false)
{
    //????��????????????
    if (number <= 0 || number > MAX_THREADS)
    {
        throw exception();
    }
    //???????????��???1??
    cout << "threadPool create " << number << " threads" << endl;
    for (int i = 0; i < number; ++i)
    {
        work_threads.emplace_back(worker, this);
    }
}
template <typename T>
threadPool<T>::~threadPool()
{
    //??stop???????????
    unique_lock<mutex> lk(pool_mutex);
    stop = true;
    //?????????????
    for (auto &wt : work_threads)
    {
        wt.join();
    }
    cout << "ThreadPool is already closed" << endl;
}
template <typename T>
bool threadPool<T>::appand(T *request)
{
    task_queue.push(request);
    return true;
}
template <typename T>
void *threadPool<T>::worker(void *arg)
{
    //???????
    threadPool *pool = (threadPool *)arg;
    pool->run();
    return pool;
}
template <typename T>
void threadPool<T>::run()
{
    //??��????????????
    while (!stop)
    {
        T *request;
        if (this->task_queue.pop(&request))
        {
            cout << "thread" << this_thread::get_id() << ":" << endl;
            request->process();
            delete request;
        }
    }
}
class Request
{
public:
    virtual void process() = 0;
};
#endif
