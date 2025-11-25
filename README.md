# 基于Reactor网络模型的KV存储演示系统

该项目整合 [webserver](https://github.com/UtopianYouth/webserver) 和 [kvstore](https://github.com/UtopianYouth/kvstore)，基于**Reactor网络模型 + 线程池**架构，实现了一个高性能、支持Web界面交互的键值存储演示系统。系统能够解析HTTP请求（GET/POST）和KV命令，提供三种不同数据结构的存储引擎，并通过RESTful API与前端实现数据交互。

## 一、项目简介

### 1.1 技术栈

**后端**

| 技术类别 | 技术选型 | 说明 |
|---------|---------|------|
| **编程语言** | C++17 | 使用现代C++特性，包括智能指针、Lambda表达式、函数封装器等 |
| **网络模型** | Reactor模式 | 主线程负责I/O多路复用，工作线程处理业务逻辑 |
| **I/O多路复用** | epoll (ET模式) | 边缘触发 + 非阻塞I/O，实现高并发网络通信 |
| **并发处理** | 线程池 | 基于阻塞队列 + 条件变量实现，避免频繁创建/销毁线程 |
| **HTTP解析** | 有限状态机 | 主从状态机配合，高效解析HTTP请求行/请求头/请求体 |
| **定时器** | 升序双向链表 | 管理非活跃连接，定期清理超时客户端 |
| **线程同步** | 读写锁 (shared_mutex) | 细粒度锁保护KV数据结构，读操作并发，写操作互斥 |
| **存储引擎** | Array / Hash / RBTree | 三种数据结构实现，满足不同场景需求 |
| **内存管理** | mmap + writev | 零拷贝技术，高效处理静态文件响应 |
| **数据交互** | JSON | 前后端基于JSON格式通信 |

**前端**

- **HTML5 + CSS3 + JavaScript**：响应式设计，毛玻璃效果，流畅动画
- **RESTful API**：标准HTTP接口，支持CORS跨域

### 1.2 关键技术

> - 高并发：采用epoll边缘触发模式 + 线程池，支持10K+并发连接；
> - 零拷贝：使用mmap内存映射 + writev分散写，减少数据拷贝开销；
> - 非阻塞I/O：全异步处理，避免线程阻塞，提升系统吞吐量；
>
> - 细粒度锁：每个KV存储引擎独立配备读写锁（std::shared_mutex）；
> - 读写分离：读操作使用共享锁并发执行，写操作使用独占锁保证数据一致性；
> - 无锁优化：不同存储引擎的操作可并发执行，互不干扰；
>
> - 分层架构：网络层（主线程）、业务逻辑层（线程池）、存储层（kv数据结构）清晰分离，易于扩展维护；
> - 面向对象：继承HttpConnection基类实现HttpKvsConnection，复用HTTP解析逻辑；
> - 状态机解析：主从状态机配合，逐行解析HTTP协议，健壮性强。
>

### 1.3 运行环境

- **操作系统**：Linux (推荐 Ubuntu 20.04+)
- **编译器**：GCC/G++ 9.0+ (支持C++17)
- **构建工具**：GNU Make

### 1.4 编译运行步骤

```bash
# 1. 进入到项目根目录，编译项目
make

# 2. 使用make run默认运行在8080端口上
make run 

# 2. 或指定port运行 
./bin/kv-webserver [port]

# 3. 在浏览器输入指定的url访问即可 ==> http://[your_ip]:[your_port]
```

### 1.5 支持的命令
> 基于Array实现的KV存储
>
> ```bash
> SET key value		# 添加键值对
> GET key					# 获取对应键值对的值
> DEL key 				# 删除键值对
> MOD key					# 修改指定键的值
> EXIST key 			# 判断键是否存在
> ```
>
> 基于Hash实现的KV存储
>
> ```bash
> HSET key value		# 添加键值对
> HGET key					# 获取对应键值对的值
> HDEL key 				# 删除键值对
> HMOD key					# 修改指定键的值
> HEXIST key 			# 判断键是否存在
> ```
>
> 基于RBTree实现的KV存储
>
> ```bash
> RSET key value		# 添加键值对
> RGET key					# 获取对应键值对的值
> RDEL key 				# 删除键值对
> RMOD key					# 修改指定键的值
> REXIST key 			# 判断键是否存在
> ```


## 二、后端API接口

当HTTP请求是POST时，前后端交互基于JSON数据格式。

#### 2.1 执行KV命令

```json
POST /api/kv
Content-Type: application/json

{
  "cmd": "SET",
  "key": "name",
  "value": "alice"
}
```

#### 2.2 获取统计信息
```
GET /api/stats
```

响应示例
```json
{
  "status": "OK",
  "data": {
    "array": {"count": 10, "max": 1000, "remaining": 990},
    "hash": {"count": 5, "max": 1000, "remaining": 995},
    "rbtree": {"count": 8, "max": 1000, "remaining": 992}
  }
}
```

## 三、总体结构

### 3.1 前后端分离

项目采用**前后端完全分离**的架构设计，前端静态资源由Nginx服务器托管，后端专注于API服务和KV存储业务逻辑处理。

```
┌──────────────────────────────────────────────────────────────────────┐
│                         客户端浏览器                                   │
│              (发送HTTP请求 / 接收HTML+CSS+JS)                         │
└────────────────────────┬─────────────────────────────────────────────┘
                         │ HTTPS (Nginx反向代理)
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│                       Nginx服务器 (前端容器)                           │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │  静态资源服务                                                    │  │
│  │  - GET /kv/         → 返回前端页面 (index.html)                │  │
│  │  - GET /kv/*.css    → 返回样式文件 (style.css)                 │  │
│  │  - GET /kv/*.js     → 返回脚本文件 (app.js)                    │  │
│  │  - GET /kv/imgs/*   → 返回图片资源                              │  │
│  └────────────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │  API反向代理                                                     │  │
│  │  - POST /kv/api/kv   → proxy_pass http://kv-backend:8080/api/kv│  │
│  │  - GET /kv/api/stats → proxy_pass http://kv-backend:8080/api/.│  │
│  └────────────────────────────────────────────────────────────────┘  │
└──────────────────────────┬───────────────────────────────────────────┘
                           │ HTTP + JSON (Docker内部网络)
                           ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    C++后端服务器 (后端容器)                            │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                     主线程 (I/O线程)                             │  │
│  │  ┌──────────────────────────────────────────────────────────┐  │  │
│  │  │  epoll (边缘触发 + 非阻塞I/O)                             │  │  │
│  │  │  - 监听socket: 接受新连接                                 │  │  │
│  │  │  - 客户端socket: EPOLLIN/EPOLLOUT事件                     │  │  │
│  │  │  - 定时器信号管道: SIGALRM信号处理 (超时清理)             │  │  │
│  │  └──────────────────────────────────────────────────────────┘  │  │
│  │           │                          │                          │  │
│  │           │ 新连接                    │ API请求就绪              │  │
│  │           ▼                          ▼                          │  │
│  │  创建HttpKvsConnection        读取完整HTTP请求                  │  │
│  │  对象并注册到epoll            将process()加入线程池             │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                                       │                               │
│                                       ▼                               │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                  线程池 (工作线程池)                             │  │
│  │  ┌──────────────────────────────────────────────────────────┐  │  │
│  │  │  阻塞队列 (生产者-消费者模型)                             │  │  │
│  │  │  - 条件变量: 队列空时工作线程阻塞等待                      │  │  │
│  │  │  - 互斥锁: 保护队列的线程安全                             │  │  │
│  │  └──────────────────────────────────────────────────────────┘  │  │
│  │           │ 取出任务 (std::function<void()>)                   │  │
│  │           ▼                                                     │  │
│  │  ┌──────────────────────────────────────────────────────────┐  │  │
│  │  │  HttpKvsConnection::process()                            │  │  │
│  │  │  1. 有限状态机解析HTTP请求                                │  │  │
│  │  │  2. API路由判断:                                          │  │  │
│  │  │     - POST /api/kv    → processKvsRequest()             │  │  │
│  │  │     - GET /api/stats  → kvs_get_stats()                 │  │  │
│  │  │     - 其他路径         → writeNotFoundResponse() (404)   │  │  │
│  │  │  3. 生成JSON响应 (状态行+响应头+JSON数据)                 │  │  │
│  │  └──────────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                                       │                               │
│                                       ▼                               │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                    存储引擎层                                    │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │  │
│  │  │  Array引擎   │  │  Hash引擎    │  │ RBTree引擎   │         │  │
│  │  │  + 读写锁    │  │  + 读写锁    │  │  + 读写锁    │         │  │
│  │  │(shared_mutex)│  │(shared_mutex)│  │(shared_mutex)│         │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘         │  │
│  │       O(n)              O(1)              O(log n)             │  │
│  └────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────┘
```

前后端分离之后，优势如下：

| 优势         | 说明                                                    |
| ------------ | ------------------------------------------------------- |
| **职责分离** | 前端专注页面渲染，后端专注业务逻辑，代码更清晰          |
| **性能提升** | Nginx处理静态文件性能远超C++，后端线程池资源100%用于API |
| **独立扩展** | 前端可部署到CDN，后端可水平扩展多实例                   |
| **开发效率** | 前后端可并行开发，互不阻塞                              |
| **容器优化** | 后端镜像更小(434KB vs 原1.2MB)，构建更快                |
| **安全性**   | 后端不再处理文件路径遍历风险，减少攻击面                |

### 3.2 数据流与请求处理

#### 3.2.1 前端静态资源请求流程

```
用户访问 https://www.utopianyouth.top/kv/
              │
              ▼
    Nginx服务器 (监听443端口)
              │
              ├─ 匹配 location /kv/
              │         │
              │         ▼
              │  读取 /usr/share/nginx/html/kv/index.html
              │         │
              │         ▼
              │  返回HTML页面 + CSS + JS资源
              │
              └─ 响应: 200 OK (静态文件直接返回)
```

#### 3.2.2 API请求流程 (KV命令执行)

```
前端发起POST请求: /kv/api/kv
              │
              │ {"cmd":"SET","key":"name","value":"alice"}
              │
              ▼
    Nginx反向代理 (重写URL: /kv/api/kv → /api/kv)
              │
              │ proxy_pass http://kv-backend:8080/api/kv
              │
              ▼
    C++后端服务器:8080
              │
              ├─ epoll检测到EPOLLIN事件
              │         │
              │         ▼
              │  主线程读取HTTP请求到缓冲区
              │         │
              │         ▼
              │  将process()任务加入线程池队列
              │         │
              │         ▼
              │  工作线程从队列取出任务
              │         │
              │         ├─ processRead() 状态机解析HTTP请求
              │         │         ├─ 请求行: POST /api/kv HTTP/1.1
              │         │         ├─ 请求头: Content-Type, Content-Length
              │         │         └─ 请求体: JSON数据
              │         │
              │         ├─ process() 路由判断
              │         │         └─ 匹配到 POST /api/kv
              │         │
              │         ├─ processKvsRequest() 处理KV命令
              │         │         ├─ parseJsonField() 解析JSON
              │         │         ├─ kvs_handle_command() 调用存储引擎
              │         │         │         ├─ 根据命令前缀路由 (SET→Array, HSET→Hash, RSET→RBTree)
              │         │         │         ├─ 获取对应引擎的读写锁
              │         │         │         ├─ 执行存储操作 (增/删/改/查)
              │         │         │         └─ 返回JSON结果
              │         │         └─ writeJsonResponse() 生成HTTP响应
              │         │
              │         └─ 写入响应缓冲区 (m_write_buf)
              │
              ├─ epoll检测到EPOLLOUT事件
              │         │
              │         ▼
              │  主线程通过writev()写回响应
              │
              ▼
    返回JSON响应给Nginx
              │
              ▼
    Nginx转发响应给浏览器
              │
              ▼
    前端接收: {"status":"OK","message":"操作成功"}
```

#### 3.2.3 统计信息请求流程

```
前端发起GET请求: /kv/api/stats
              │
              ▼
    Nginx反向代理 → C++后端:8080
              │
              ├─ 匹配到 GET /api/stats
              │         │
              │         ▼
              │  kvs_get_stats() 遍历三个存储引擎
              │         │
              │         ├─ global_array: count, max, remaining
              │         ├─ global_hash: count, max, remaining
              │         └─ global_rbtree: count, max, remaining
              │
              └─ 返回JSON统计数据
```

### 3.3 核心模块

#### 3.3.1 前端 (Nginx)

> - 静态资源托管：提供HTML/CSS/JS/Images等前端文件；
>- 反向代理：将API请求转发到后端C++服务器；
> - HTTPS支持：SSL证书配置，自动将HTTP重定向到HTTPS；
> - 缓存策略：静态资源缓存7天，API请求不缓存。
> 
> **关键配置**
> 
>- 路径重写：`/kv/api/*` → `/api/*` (去除前缀)；
> - 上游服务：`upstream kv_backend { server kv-backend:8080; }`；
>- 超时设置：`proxy_read_timeout 30s`。

#### 3.3.2 后端

> - 主线程初始化：创建监听socket，设置为非阻塞，注册到epoll；
> - 接受连接：epoll检测到监听socket可读，accept新连接；
> - 连接注册：为新连接创建`HttpKvsConnection`对象，设置非阻塞 + 边缘触发 + EPOLLONESHOT；
> - 数据就绪：epoll检测到客户端socket可读，主线程读取数据到缓冲区；
> - 任务派发：HTTP请求读取完整后，将`process()`封装为std::function加入线程池；
> - 响应写回：工作线程处理完成后，主线程通过epoll检测EPOLLOUT事件写回响应。
>
> **关键技术**
>
> - 边缘触发(ET) + 非阻塞I/O：避免惊群效应，提高效率；
> - EPOLLONESHOT：保证一个socket同一时刻只被一个线程处理；
> - 信号处理：SIGALRM信号通过管道通知主线程，触发定时器逻辑。

#### 3.3.3 并发层

> - 阻塞队列：生产者-消费者模型，使用条件变量 + 互斥锁实现；
> - 工作线程：启动时创建固定数量线程，循环从队列取任务执行；
> - 任务封装：使用`std::function<void()>`封装Lambda表达式，实现任意函数绑定。
>
> **优势**
>
> - 避免频繁创建/销毁线程的开销；
> - 限制并发线程数量，防止资源耗尽；
> - 支持异步任务执行，提升系统响应速度。

#### 3.3.4 HTTP解析层

> **主状态机 (CHECK_STATE)**
>
> - `CHECK_STATE_REQUESTLINE`：解析请求行 (方法 URL 版本)；
> - `CHECK_STATE_HEADER`：解析请求头 (Connection, Content-Length等)；
> - `CHECK_STATE_CONTENT`：解析请求体 (POST的JSON数据)，
>
> **从状态机 (LINE_STATUS)**
>
> - `LINE_OK`：读取到完整的一行 (\r\n)；
> - `LINE_BAD`：行格式错误；
> - `LINE_OPEN`：行数据不完整，需要继续读取。
>
> ```apl
> 读取数据 → 从状态机逐行解析 → 主状态机处理每一行
>          ↓
>     根据当前状态调用对应解析函数
>          ↓
>     状态转移 (请求行→请求头→请求体)
>          ↓
>     解析完成 → 返回GET_REQUEST → 进入API路由处理
> ```

#### 3.3.5 API路由层

> **路由策略**
>
> - `POST /api/kv`：KV命令执行接口，解析JSON请求体，调用`processKvsRequest()`；
> - `GET /api/stats`：统计信息接口，调用`kvs_get_stats()`返回三个引擎的容量统计；
> - **其他路径**：返回404 JSON错误响应，明确告知这是后端API服务器。
>
> **JSON解析**
>
> - 手工实现轻量级JSON解析器`parseJsonField()`，避免引入第三方库；
> - 提取`cmd`、`key`、`value`字段，转发给存储引擎层。
>

#### 3.3.6 存储层

| 引擎类型 | 数据结构 | 时间复杂度 | 容量 | 适用场景 | 线程安全 |
|---------|---------|-----------|------|---------|---------|
| **Array** | 线性数组 | 查找O(n), 插入O(1) | 512K | 小规模数据，测试场景 | 独立读写锁 |
| **Hash** | 链地址法哈希表 | 查找O(1), 插入O(1) | 128K桶 | 大规模快速查找 | 独立读写锁 |
| **RBTree** | 红黑树 | 查找O(log n), 插入O(log n) | 512K | 需要有序遍历 | 独立读写锁 |

> - 每个引擎独立配备`std::shared_mutex`读写锁；
> - 读操作（GET/EXIST）：使用`std::shared_lock`，多个线程可并发读；
> - 写操作（SET/DEL/MOD）：使用`std::unique_lock`，独占访问；
> - 不同引擎之间无锁竞争，可并发执行。

#### 3.3.7 定时器

> **数据结构**
>
> - 升序双向链表，按超时时间排序；
> - 每个客户端连接对应一个`UtilTimer`对象；
> - 定时器存储客户端socket信息和回调函数。
>
> **工作机制**
>
> - 新连接建立时创建定时器，超时时间 = 当前时间 + 3 * TIMESLOT；
> - 客户端有数据到达时，调用`adjustTimer()`延长超时时间；
> - SIGALRM信号触发`tick()`函数，遍历链表检查超时连接；
> - 超时连接执行回调函数`cbFunc()`，关闭socket并释放资源。

### 3.4 部署架构

#### 3.4.1 容器化部署

项目采用Docker容器化部署，通过docker-compose编排前后端服务：

```yaml
services:
  backend:                           # 后端C++服务
    build: ./backend
    image: kv-backend:latest
    container_name: kv-backend
    expose:
      - "8080"                       # 仅暴露到Docker内部网络
    restart: unless-stopped

  # Nginx容器由分布式消息推送系统的docker-compose管理
  # 通过volumes挂载本项目的前端静态文件
```

**优势**

> - 前后端独立部署：后端Docker镜像仅包含C++可执行文件，不含前端资源；
> - 资源隔离：后端容器限制CPU和内存使用，防止资源耗尽；
> - 健康检查：定期检测后端进程状态，自动重启故障容器；
> - 一键启动：`docker-compose up -d` 完成所有服务启动。

#### 3.4.2 前后端通信

```
┌─────────────────┐         ┌─────────────────┐
│  Nginx容器       │         │   C++后端容器    │
│  (chatroom项目)  │◀───────▶│   (kv-backend)  │
│  监听443端口     │  HTTP   │   监听8080端口   │
└─────────────────┘         └─────────────────┘
        │                            │
        └────── Docker内部网络 ───────┘
             (kv_backend服务发现)
```

**通信方式**

> - Nginx通过Docker内部DNS解析`kv-backend:8080`；
> - 后端8080端口不对外暴露，仅允许Nginx容器访问；
> - 用户通过HTTPS访问Nginx，Nginx反向代理到后端。

### 3.5 线程同步

#### 3.5.1 问题

原kvstore项目基于**单线程 + 事件回调**的Reactor模型，未考虑多线程并发访问KV数据结构的线程安全问题。本项目采用**Reactor + 线程池**后，多个工作线程会并发访问全局KV数据结构（`global_array`、`global_hash`、`global_rbtree`），存在以下线程安全风险。

> - 数据竞争：多线程同时读写同一个键值对；
> - 内存损坏：链表/树结构在并发修改时指针混乱；
> - 数据不一致：读线程可能读到写线程修改到一半的脏数据。
>

#### 3.5.2 解决

> - 细粒度锁：每个KV引擎独立配备读写锁，避免不必要的锁竞争；
> - 读写分离：读操作使用共享锁并发执行，写操作使用独占锁；
> - 函数内加锁：锁的添加位置在每个数据结构的操作函数内部，而非`kvs_handler`层；
> - 性能优先：不同引擎之间无锁依赖，可并发执行。

## 四、测试

| 指标 | 数值 | 说明 |
|------|------|------|
| **并发连接数** | 10000+ | 基于epoll的高并发支持 |
| **QPS** | 10K~15K | 压测工具测试结果 |
| **响应延迟** | <10ms | 非阻塞I/O保证低延迟 |
| **线程池大小** | 4个工作线程 | 可配置，默认8线程 |
| **存储容量** | Array:512K, Hash:128K, RBTree:512K | 可通过宏定义调整 |

## 五、碎碎念

### 5.1 总结

本项目是一个完整的高性能KV存储演示系统，涵盖了Linux网络编程、并发编程、数据结构与算法等技术栈。

### 5.2 亮点

>  **Reactor&线程池**：主线程I/O复用 + 工作线程异步处理，充分利用多核CPU；
>  **零拷贝技术**：mmap + writev减少数据拷贝，提升静态文件响应性能；
>  **细粒度锁**：读写锁 + 独立锁策略，平衡并发性能与线程安全；
>  **状态机解析**：健壮的HTTP协议解析，支持长连接和短连接；
>  **多引擎存储**：提供三种不同特性的KV引擎，满足不同场景需求。

### 5.3 优化

优化就是不断的抄Redis存在的扩展功能，感觉以下功能不错诶。

> - [ ] 支持持久化存储（日志文件、数据快照）；
> - [ ] 实现主从复制和数据备份机制；
> - [ ] 支持更多数据类型（List、Set、Sorted Set）；
> - [ ] 引入协程优化并发性能。
>

## 六、部署

原来利用Nginx进行项目部署的时候，是可以免费进行HTTPS协议升级的，现在才知道（2025.11.24）......

由于只有一台阿里云服务器和一个域名，所以这里要进行两个项目的部署，需要修改之前的Nginx配置文件，且Nginx进行代理的时候，不同项目的区分，使用在域名后面加上资源定位符的方法，这里记录一下在部署过程中的一些重要步骤。

### 6.1 部署思路

由于基于Reactor模式的kv存储演示系统前端项目非常简单，不需要像分布式消息推送系统的前端项目那样，需要`Node.js`服务器来运行，所以本项目的所有前端静态文件，都放到了云服务器的Nginx镜像中，Nginx服务器可以通过修改配置文件直接访问，正常来说，**一台云服务器运行一个Nginx镜像就可以了**。

> **有了上述原因的描述，基本的部署思路如下**
>
> - 后端部分（kv-backend）：也就是该项目的后端部分，通过Dockerfile制作成镜像，使用单独的容器编排工具docker-compose启动（该项目优化后，前后端完全分离了，后端项目专门负责HTTP通信和kv相关的业务逻辑，不需要响应前端静态文件）；
> - 前端部分（kv-frontend）：考虑到一个云服务器运行一个Nginx镜像，但是我需要部署两个项目，所以我修改了原分布式实时消息推送系统的**docker容器编排文件**和**Nginx镜像运行的配置文件**，最后分布式实时消息推送系统的docker-compose容器编排工具也负责该项目的前端内容挂载。
>
> **修改后的Nginx配置文件如下**
>
> ```nginx
> events {}
> 
> http {
>     # 保证css加载
>     include /etc/nginx/mime.types;
>     default_type application/octet-stream;
> 
>     # 上游服务定义
>     upstream chatroom_backend {
>         server chatroom-app:8080;
>     }
> 
>     upstream chatroom_frontend {
>         server web-frontend:3000;
>     }
> 
>     upstream kv_backend {
>         server kv-backend:8080;
>     }
> 
>     # HTTP 重定向到 HTTPS
>     server {
>         listen 80;
>         server_name www.utopianyouth.top utopianyouth.top;
>         return 301 https://$server_name$request_uri;
>     }
> 
>     # HTTPS 服务器
>     server {
>         listen 443 ssl http2;
>         server_name www.utopianyouth.top utopianyouth.top;
> 
>         # SSL 证书配置
>         ssl_certificate /etc/letsencrypt/live/www.utopianyouth.top/fullchain.pem;
>         ssl_certificate_key /etc/letsencrypt/live/www.utopianyouth.top/privkey.pem;
> 
>         # SSL 安全配置
>         ssl_protocols TLSv1.2 TLSv1.3;
>         ssl_ciphers HIGH:!aNULL:!MD5;
>         ssl_prefer_server_ciphers on;
> 
>         # Gzip压缩配置
>         gzip on;
>         gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;
>         gzip_min_length 1000;
> 
>         # KV项目 - /kv/ 路径
>         location /kv/api/ {
>             rewrite ^/kv(/api/.*)$ $1 break;
> 
>             proxy_pass http://kv_backend;
>             proxy_set_header Host $host;
>             proxy_set_header X-Real-IP $remote_addr;
>             proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
>             proxy_set_header X-Forwarded-Proto $scheme;
> 
>             proxy_connect_timeout 30s;
>             proxy_send_timeout 30s;
>             proxy_read_timeout 30s;
> 
>             add_header Cache-Control "no-cache, no-store, must-revalidate";
>             add_header Pragma "no-cache";
>             add_header Expires "0";
>         }
> 
>         location /kv/ {
>             alias /usr/share/nginx/html/kv/;
>             index index.html;
>             try_files $uri $uri/ /kv/index.html;
> 
>             location ~* \.(css|js|jpg|jpeg|png|gif|ico|svg)$ {
>                 expires 7d;
>                 add_header Cache-Control "public, immutable";
>             }
>         }
> 
>         # Chatroom的API请求和WebSocket
>         location /api/ {
>             proxy_pass http://chatroom_backend;
> 
>             proxy_http_version 1.1;
>             proxy_set_header Upgrade $http_upgrade;
>             proxy_set_header Connection "Upgrade";
>             proxy_set_header Host $host;
> 
>             proxy_read_timeout 86400s;
>             proxy_send_timeout 86400s;
>             proxy_connect_timeout 75s;
> 
>             proxy_buffering off;
>         }
> 
>         # Chatroom的前端应用
>         location / {
>             proxy_pass http://chatroom_frontend;
>             proxy_set_header Host $host;
>         }
> 
>         # 错误页面
>         error_page 500 502 503 504 /50x.html;
>         location = /50x.html {
>             return 500 "Backend service unavailable";
>         }
>     }
> }
> ```
>
> 

### 6.2 部署要求

由于两个项目部署完成之后，需要支持域名解析以及HTTPS协议，所以有了云服务器之后，还需要如下硬性要求：

> - 已有域名并正确解析到服务器IP；
> - OS中安装Certbot插件，以支持HTTPS。
>

在Ubuntu中，HTTPS协议的具体配置方式如下：

```bash
# 1.安装Certbot
sudo apt update
sudo apt install certbot python3-certbot-nginx -y
certbot --version

# 2.申请SSL证书
sudo certbot certonly --standalone -d [your_domain]

# 3. 证书申请成功后，输出如下内容
Successfully received certificate.
Certificate is saved at: /etc/letsencrypt/live/[your_domain]/fullchain.pem
Key is saved at: /etc/letsencrypt/live/[your_domain]/privkey.pem

# 4. 证书有效期默认90天
```

最后我的项目部署成功了，欢迎访问使用：

分布式实时消息推送系统：[https://www.utopianyouth.top](https://www.utopianyouth.top) ;

基于Reactor网络模型的kv存储演示系统：[https://www.utopianyouth.top/kv/](https://www.utopianyouth.top/kv/) .

(https://github.com/UtopianYouth/RHEEDAnalysis)[https://github.com/UtopianYouth/RHEEDAnalysis]















