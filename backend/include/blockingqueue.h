#pragma once
#include <condition_variable>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>

template <typename T>
class BlockingQueue {
private:
    bool m_nonblock;                // 控制线程退出 
    std::queue<T> m_queue;          // 任务队列
    std::mutex m_mutex;             // 互斥锁
    std::condition_variable m_cond; // 条件变量
public:
    void Push(const T& value);      // 入队
    bool Pop(T& value);             // 出队
    void Cancel();                  // 唤醒阻塞在工作队列中的线程，如果工作队列为空，线程退出
};


// 减小锁的细粒度，不让生产者和消费者频繁的竞争同一个互斥锁 ==> 提高并发量
template <typename T>
class BlockingQueuePro {
private:
    bool m_nonblock;
    std::queue<T> m_producer_q;
    std::queue<T> m_consumer_q;
    std::mutex m_producer_mtx;
    std::mutex m_consumer_mtx;
    std::condition_variable m_cond;
    int SwapQueue();
public:
    void Push(const T& value);
    bool Pop(T& value);
    void Cancel();                  // 唤醒阻塞在工作队列中的线程，如果消费者队列和生产者队列为空，线程退出
};


// 模板类的定义和实现放在头文件中
// =============== BlockingQueue =======================
template<typename T>
void BlockingQueue<T>::Push(const T& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(value);
    m_cond.notify_one();    // 唤醒一个被阻塞的线程
}

template<typename T>
bool BlockingQueue<T>::Pop(T& value) {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cond.wait(lock, [this]() -> bool {
        // true: next(awaken)
        // false: block(sleep)
        return !m_queue.empty() || m_nonblock;      // 队列为空，阻塞且释放互斥锁
        });

    if (m_queue.empty()) {
        return false;       // 线程退出
    }

    value = m_queue.front();
    m_queue.pop();

    return true;
}

template<typename T>
void BlockingQueue<T>::Cancel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nonblock = true;
    m_cond.notify_all();
}

// =============== BlockingQueuePro =======================
template<typename T>
int BlockingQueuePro<T>::SwapQueue() {
    std::unique_lock<std::mutex> lock(m_producer_mtx);
    m_cond.wait(lock, [this]()->bool {
        return !m_producer_q.empty() || m_nonblock;
        });
    std::swap(m_producer_q, m_consumer_q);
    return m_consumer_q.size();
}

template<typename T>
void BlockingQueuePro<T>::Push(const T& value) {
    std::lock_guard<std::mutex> lock(m_producer_mtx);
    m_producer_q.push(value);
    m_cond.notify_one();    // 唤醒一个被阻塞的线程
}

template<typename T>
bool BlockingQueuePro<T>::Pop(T& value) {
    std::unique_lock<std::mutex> lock(m_consumer_mtx);
    if (m_consumer_q.empty() && SwapQueue() == 0) {
        return false;
    }

    value = m_consumer_q.front();
    m_consumer_q.pop();
    return true;
}

template<typename T>
void BlockingQueuePro<T>::Cancel() {
    std::lock_guard<std::mutex> lock(m_producer_mtx);
    m_nonblock = true;
    m_cond.notify_all();    // 通知阻塞的所有线程退出
}
