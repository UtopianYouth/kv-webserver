#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include "threadpool.h"
#include "http_kvs_connection.h"
#include "lst_timer.h"
#include "kvs_handler.h"

#define MAX_FD 65535                // 支持最大的文件描述符个数
#define MAX_EVENT_NUMBER 65535      // epoll监听的最大的IO事件数
#define TIMESLOT 5                  // 定时器信号间隔时间
#define MAX_THREADS 4               // 线程池最大的线程数量

static int pipefd[2];               // 定时器发送信号通过管道传输，0是读端，1是写端
static SortTimerLst timer_lst;      // 定时器双向链表，一个TCP连接对应一个定时器
static int epoll_fd = 0;            // epoll_create()
HttpKvsConnection* users = new HttpKvsConnection[MAX_FD];     // 客户端的TCP连接任务类对象
ClientData* lst_users = new ClientData[MAX_FD];         // 定时器客户端信息类对象

// 添加信号捕捉
void addSignal(int sig, void(handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);    // 设置阻塞信号集
    assert(sigaction(sig, &sa, NULL) != -1);
}

// 添加信号处理函数，管道写端
void sigHandler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

// 超时函数处理
void timerHandler() {
    // 定时处理任务，实际上就是调用tick()函数
    timer_lst.tick();
    // 因为一次alarm调用只会引起一次SIGALRM信号，所以要重新定时，发送SIGALRM信号
    alarm(TIMESLOT);
}

// 定时器回调函数，删除超时连接的socket上的注册事件
extern void cbFunc(ClientData* user_data) {
    users[user_data->sockfd].closeConnection();
}

// 设置文件描述符非阻塞
extern int setNonBlocking(int fd);

// 添加文件描述符到epoll对象中
extern void addFDEpoll(int epoll_fd, int fd, bool et, bool one_shot);

// 从epoll对象中删除文件描述符
extern void removeFDEpoll(int epoll_fd, int fd);

// 修改epoll对象中的文件描述符
extern void modifyFDEpoll(int epoll_fd, int fd, int event_num);

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Usage: %s port_number\n", basename(argv[0]));
        exit(-1);
    }

    // 获取端口号
    int port = atoi(argv[1]);

    // 初始化KV存储
    if (init_kvengine() != 0) {
        printf("Failed to initialize KV storage engines!\n");
        exit(-1);
    }
    printf("kv storage engines initialized successfully.\n");

    // 对 SIGPIPE 信号进行处理
    addSignal(SIGPIPE, SIG_IGN);
    addSignal(SIGALRM, sigHandler);
    addSignal(SIGTERM, sigHandler);
    bool stop_server = false;

    // 创建epoll事件数组
    epoll_event events[MAX_EVENT_NUMBER];

    // 创建epoll fd对象
    epoll_fd = epoll_create(5);

    ThreadPool* pool = nullptr;
    try {
        pool = new ThreadPool(MAX_THREADS);
        printf("Thread pool created with %d threads.\n", MAX_THREADS);
    }
    catch (...) {
        printf("Failed to create thread pool!\n");
        exit(-1);
    }

    // 创建一对相互连接的匿名套接字，适用于本地IPC，支持全双工通信
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);

    setNonBlocking(pipefd[1]);
    addFDEpoll(epoll_fd, pipefd[0], false, false);

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 端口复用
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret1 = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret1 == -1) {
        perror("bind");
        exit(-1);
    }

    // 监听
    int ret2 = listen(listen_fd, 65535);
    if (ret2 == -1) {
        perror("listen");
        exit(-1);
    }

    // listenfd ==> epoll
    addFDEpoll(epoll_fd, listen_fd, false, false);

    // 初始化 HttpConnection 的 static 参数
    HttpConnection::m_epoll_fd = epoll_fd;

    bool timeout = false;
    alarm(TIMESLOT);

    printf("kv webserver started on port %d\n", port);

    // 检测 epoll 对象中的 IO 缓冲区变化
    while (!stop_server) {
        int num = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if ((num < 0) && (errno != EINTR)) {
            // 被中断，或者 epoll_wait() 出错
            printf("epoll failure.\n");
            break;
        }

        // 遍历epoll对象事件数组
        for (int i = 0; i < num; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listen_fd) {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int communication_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
                if (communication_fd < 0) {
                    perror("accept");
                    printf("errno is %d.\n", errno);
                    continue;
                }

                if (HttpConnection::m_user_count >= MAX_FD) {
                    // 客户端的连接数已满
                    close(communication_fd);
                    continue;
                }

                // 客户端连接初始化
                users[communication_fd].init(communication_fd, client_addr);

                // 定时器用户初始化
                lst_users[communication_fd].address = client_addr;
                lst_users[communication_fd].sockfd = communication_fd;

                // 创建定时器
                UtilTimer* timer = new UtilTimer;
                timer->user_data = &lst_users[communication_fd];
                timer->cb_func = cbFunc;
                time_t cur = time(NULL);    // 获取当前系统时间
                timer->expire = cur + 3 * TIMESLOT;
                lst_users[communication_fd].timer = timer;
                timer_lst.addTimer(timer);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sockfd].closeConnection();
            }
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                // 处理信号
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1 || ret == 0) {
                    continue;
                }
                else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                        case SIGALRM:
                            // 用timeout标记有定时任务需要处理，但不立即处理定时任务
                            timeout = true;
                            break;
                        case SIGTERM:
                            stop_server = true;
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLIN) {
                // 通信文件描述符读缓冲区有数据
                UtilTimer* timer = lst_users[sockfd].timer;
                if (users[sockfd].read()) {
                    int fd = sockfd;
                    pool->Post([fd]()->void { users[fd].process(); });

                    // 成功读取数据，更新定时器
                    if (timer) {
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        timer_lst.adjustTimer(timer);
                    }
                }
                else {
                    // 客户端关闭连接，移除定时器
                    if (timer) {
                        timer_lst.delTimer(timer);
                    }
                    users[sockfd].closeConnection();
                }
            }
            else if (events[i].events & EPOLLOUT) {
                if (!users[sockfd].write()) {
                    // 如果客户端的 keep-alive = false，只写一次 HTTP 响应
                    users[sockfd].closeConnection();
                }
            }
        }

        // 处理定时事件，I/O有更高优先级
        if (timeout) {
            timerHandler();
            timeout = false;
        }
    }

    close(epoll_fd);
    close(listen_fd);
    close(pipefd[1]);
    close(pipefd[0]);

    // 工作任务对象
    delete[] users;

    // 定时器用户信息对象
    delete[] lst_users;

    // 线程池对象
    delete pool;

    destroy_kvengine();

    return 0;
}
