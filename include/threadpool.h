#pragma once
#include <thread>
#include <functional>
#include <vector>

// 前置声明，解决循环依赖的问题 ==> #include "blockingqueue.h"
// 前置声明的类只能用做指针或引用，如果是一个非指针成员变量，需要执行构造函数
template <typename T>
class BlockingQueue;

template <typename T>
class BlockingQueuePro;

class ThreadPool {
private:

    void Worker();      // 线程运行函数
    // std::function<void()> 函数封装器
    // unique_ptr<T> 防止线程池对象浅拷贝问题
    std::unique_ptr<BlockingQueuePro<std::function<void()>>> m_task_queue;     // 工作队列
    std::vector<std::thread> m_threads;     // 线程池数组
public:

    explicit ThreadPool(size_t threads_num);   // 初始化线程池
    ~ThreadPool();                          // 销毁线程池
    void Post(std::function<void()> task);  // 发布任务到线程池
};
