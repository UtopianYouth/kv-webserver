# Docker 部署指南

本文档介绍如何使用Docker部署KV存储演示系统到云服务器。

## 目录

- [前置要求](#前置要求)
- [方式一：使用Docker命令](#方式一使用docker命令)
- [方式二：使用Docker Compose](#方式二使用docker-compose)
- [云服务器部署](#云服务器部署)
- [常见问题](#常见问题)

---

## 前置要求

### 1. 安装Docker

**Ubuntu/Debian:**
```bash
# 更新包索引
sudo apt-get update

# 安装依赖
sudo apt-get install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release

# 添加Docker官方GPG密钥
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

# 设置Docker仓库
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# 安装Docker Engine
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# 验证安装
docker --version
```

**CentOS/RHEL:**
```bash
# 安装依赖
sudo yum install -y yum-utils

# 添加Docker仓库
sudo yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo

# 安装Docker Engine
sudo yum install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# 启动Docker服务
sudo systemctl start docker
sudo systemctl enable docker

# 验证安装
docker --version
```

### 2. 安装Docker Compose（如果使用方式二）

```bash
# Docker Compose v2已集成到Docker中，使用docker compose命令
# 如果需要独立安装Docker Compose v1:
sudo curl -L "https://github.com/docker/compose/releases/download/v2.20.0/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
docker-compose --version
```

### 3. 配置Docker用户权限（可选）

```bash
# 将当前用户添加到docker组，避免每次使用sudo
sudo usermod -aG docker $USER

# 重新登录或执行以下命令使权限生效
newgrp docker
```

---

## 方式一：使用Docker命令

### 1. 构建Docker镜像

在项目根目录下执行：

```bash
# 构建镜像，标签为kv-webserver:latest
docker build -t kv-webserver:latest .

# 查看镜像
docker images | grep kv-webserver
```

**构建过程说明：**
- 多阶段构建：先在builder阶段编译项目，再复制二进制文件到运行镜像
- 最终镜像大小约 **100-150MB**，精简高效
- 构建时间约 **2-5分钟**（取决于网络速度）

### 2. 运行容器

```bash
# 运行容器，映射8080端口
docker run -d \
  --name kv-webserver \
  -p 8080:8080 \
  --restart unless-stopped \
  kv-webserver:latest

# 查看运行状态
docker ps

# 查看日志
docker logs kv-webserver

# 实时查看日志
docker logs -f kv-webserver
```

### 3. 验证部署

```bash
# 在服务器上测试
curl http://localhost:8080

# 从浏览器访问
# http://your-server-ip:8080
```

### 4. 容器管理命令

```bash
# 停止容器
docker stop kv-webserver

# 启动容器
docker start kv-webserver

# 重启容器
docker restart kv-webserver

# 删除容器
docker rm -f kv-webserver

# 进入容器shell
docker exec -it kv-webserver /bin/bash

# 查看容器资源使用情况
docker stats kv-webserver
```

---

## 方式二：使用Docker Compose

### 1. 启动服务

在项目根目录下执行：

```bash
# 构建并启动容器（后台运行）
docker compose up -d

# 或使用docker-compose命令（v1）
docker-compose up -d
```

### 2. 查看状态

```bash
# 查看服务状态
docker compose ps

# 查看日志
docker compose logs

# 实时查看日志
docker compose logs -f

# 查看特定服务的日志
docker compose logs kv-webserver
```

### 3. 管理服务

```bash
# 停止服务
docker compose stop

# 启动服务
docker compose start

# 重启服务
docker compose restart

# 停止并删除容器
docker compose down

# 停止并删除容器、网络、卷
docker compose down -v

# 重新构建并启动
docker compose up -d --build
```

---

## 云服务器部署

### 1. 上传代码到云服务器

**方式A: 使用Git（推荐）**
```bash
# SSH登录云服务器
ssh user@your-server-ip

# 克隆仓库
git clone https://github.com/YourUsername/kv-webserver.git
cd kv-webserver
```

**方式B: 使用SCP上传**
```bash
# 在本地打包项目
tar -czf kv-webserver.tar.gz kv-webserver/

# 上传到服务器
scp kv-webserver.tar.gz user@your-server-ip:/home/user/

# SSH登录服务器并解压
ssh user@your-server-ip
tar -xzf kv-webserver.tar.gz
cd kv-webserver
```

### 2. 配置防火墙

**Ubuntu/Debian (使用ufw):**
```bash
# 允许8080端口
sudo ufw allow 8080/tcp

# 查看防火墙状态
sudo ufw status
```

**CentOS/RHEL (使用firewalld):**
```bash
# 允许8080端口
sudo firewall-cmd --permanent --add-port=8080/tcp
sudo firewall-cmd --reload

# 查看开放端口
sudo firewall-cmd --list-ports
```

**云服务商安全组配置：**
- 阿里云：在ECS实例的安全组中添加入站规则，开放8080端口
- 腾讯云：在云服务器的安全组中添加入站规则，开放8080端口
- AWS：在EC2实例的Security Group中添加Inbound Rule，开放8080端口

### 3. 部署容器

```bash
# 使用Docker Compose部署（推荐）
docker compose up -d

# 或使用Docker命令
docker build -t kv-webserver:latest .
docker run -d --name kv-webserver -p 8080:8080 --restart unless-stopped kv-webserver:latest
```

### 4. 设置反向代理（可选）

如果你希望使用80端口或配置HTTPS，可以使用Nginx作为反向代理。

**安装Nginx:**
```bash
sudo apt-get install -y nginx
```

**配置Nginx (`/etc/nginx/sites-available/kv-webserver`):**
```nginx
server {
    listen 80;
    server_name your-domain.com;  # 替换为你的域名或IP

    location / {
        proxy_pass http://localhost:8080;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

**启用配置:**
```bash
sudo ln -s /etc/nginx/sites-available/kv-webserver /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx
```

### 5. 配置HTTPS（可选）

使用Let's Encrypt免费SSL证书：

```bash
# 安装Certbot
sudo apt-get install -y certbot python3-certbot-nginx

# 获取SSL证书
sudo certbot --nginx -d your-domain.com

# 证书会自动续期，查看续期任务
sudo certbot renew --dry-run
```

---

## 常见问题

### Q1: 镜像构建失败，提示"unable to connect to archive.ubuntu.com"

**解决方案：**
- 网络问题，可以使用国内镜像源
- 在Dockerfile中添加：
```dockerfile
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list
```

### Q2: 容器启动后立即退出

**解决方案：**
```bash
# 查看容器日志
docker logs kv-webserver

# 常见原因：
# 1. 端口被占用 - 检查8080端口是否被其他程序占用
lsof -i:8080
# 2. 权限问题 - 确保有权限访问frontend和resources目录
```

### Q3: 无法从浏览器访问服务

**解决方案：**
1. 检查容器是否正常运行：`docker ps`
2. 检查端口映射：`docker port kv-webserver`
3. 检查防火墙：`sudo ufw status` 或 `sudo firewall-cmd --list-ports`
4. 检查云服务器安全组规则
5. 在服务器本地测试：`curl http://localhost:8080`

### Q4: 如何更新部署的应用

**方案A: 重新构建镜像**
```bash
# 拉取最新代码
git pull

# 停止并删除旧容器
docker compose down

# 重新构建并启动
docker compose up -d --build
```

**方案B: 不停机更新（使用多容器）**
```bash
# 启动新容器
docker run -d --name kv-webserver-new -p 8081:8080 kv-webserver:latest

# 验证新容器
curl http://localhost:8081

# 切换Nginx反向代理到新容器
# 删除旧容器
docker stop kv-webserver
docker rm kv-webserver
```

### Q5: 如何查看容器内的文件

```bash
# 进入容器
docker exec -it kv-webserver /bin/bash

# 查看目录结构
ls -la /app

# 退出容器
exit
```

### Q6: 如何限制容器资源使用

**使用docker run命令：**
```bash
docker run -d \
  --name kv-webserver \
  -p 8080:8080 \
  --memory="512m" \
  --cpus="1.0" \
  kv-webserver:latest
```

**使用docker-compose.yml：**
已在文件中配置，参考deploy.resources部分。

---

## 性能优化建议

### 1. 使用多阶段构建

Dockerfile已采用多阶段构建，将构建环境和运行环境分离，显著减小镜像体积。

### 2. 合理设置资源限制

根据服务器配置，在docker-compose.yml中调整CPU和内存限制：
- 单核服务器：cpus: '1.0', memory: 512M
- 双核服务器：cpus: '2.0', memory: 1G
- 四核服务器：cpus: '4.0', memory: 2G

### 3. 配置日志轮转

docker-compose.yml已配置日志轮转（max-size: 10m, max-file: 3），避免日志文件过大。

### 4. 监控容器健康状态

```bash
# 查看健康检查状态
docker inspect --format='{{.State.Health.Status}}' kv-webserver

# 查看健康检查日志
docker inspect --format='{{range .State.Health.Log}}{{.Output}}{{end}}' kv-webserver
```

---

## 备份与恢复

### 备份容器数据

```bash
# 导出镜像
docker save kv-webserver:latest | gzip > kv-webserver-image.tar.gz

# 备份日志
docker logs kv-webserver > kv-webserver.log 2>&1
```

### 恢复镜像

```bash
# 导入镜像
docker load < kv-webserver-image.tar.gz
```

---

## 监控和维护

### 查看容器资源使用

```bash
# 实时监控
docker stats kv-webserver

# 查看详细信息
docker inspect kv-webserver
```

### 清理Docker资源

```bash
# 清理停止的容器
docker container prune

# 清理未使用的镜像
docker image prune -a

# 清理未使用的网络
docker network prune

# 清理所有未使用的资源
docker system prune -a
```

---

## 总结

本文档提供了完整的Docker部署流程，包括本地测试和云服务器部署。建议：

1. **开发环境**：使用Docker Compose进行快速迭代
2. **生产环境**：配置Nginx反向代理 + HTTPS
3. **定期备份**：导出镜像和日志文件
4. **监控运维**：使用`docker stats`监控资源使用情况

有任何问题，请参考[主README文档](../README.md)或提交Issue。
