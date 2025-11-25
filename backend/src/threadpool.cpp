#include "threadpool.h"
#include "blockingqueue.h"
#include<memory>

// 初始化线程池
ThreadPool::ThreadPool(size_t threads_num) {
    m_task_queue = std::make_unique<BlockingQueuePro<std::function<void()>>>();
    for (size_t i = 0;i < threads_num;++i) {
        // 创建线程，基于Cpp
        m_threads.emplace_back([this]()-> void {Worker();});
    }
}

// 线程逻辑函数，从工作队列中取出任务
void ThreadPool::Worker() {
    while (1) {
        std::function<void()> task;
        if (!m_task_queue->Pop(task)) {
            break;
        }
        task();
    }
}

// 向工作队列中加入任务
void ThreadPool::Post(std::function<void()> task) {
    m_task_queue->Push(task);
}

// 销毁线程池
ThreadPool::~ThreadPool() {

    m_task_queue->Cancel();

    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();  // 主线程等待子线程执行结束，让OS回收子线程资源
        }
    }
}