# 部署脚本使用指南

本目录包含用于构建、推送和部署 kv-webserver 项目的自动化脚本。

## 脚本列表

### 1. push-to-registry.sh
**功能**: 构建并推送镜像到阿里云容器镜像仓库

**使用方法**:
```bash
# 推送 latest 标签（默认）
./scripts/push-to-registry.sh

# 推送指定标签
./scripts/push-to-registry.sh v1.0.0
```

**脚本流程**:
1. 检查并加载 `.env` 配置
2. 检查本地镜像，询问是否重新构建
3. 为镜像打标签
4. 检查 Docker 登录状态
5. 推送镜像到远程仓库
6. 显示部署命令

**前置要求**:
- 已创建 `.env` 文件
- 已登录 Docker 仓库（脚本会自动检查并提示登录）

---

### 2. deploy-to-server.sh
**功能**: 一键部署项目到云服务器

**使用方法**:
```bash
# 部署到云服务器
./scripts/deploy-to-server.sh user@server-ip

# 示例
./scripts/deploy-to-server.sh root@192.168.1.100
```

**脚本流程**:
1. 测试 SSH 连接
2. 在服务器上创建项目目录
3. 上传前端静态文件
4. 上传配置文件（.env, docker-compose.prod.yml）
5. 上传 nginx 统一配置
6. 在服务器上拉取并启动容器

**前置要求**:
- 已配置 SSH 免密登录（推荐）
- 镜像已推送到远程仓库
- 服务器已安装 Docker 和 docker-compose

---

## 完整部署流程

### 第一步: 配置环境变量

```bash
# 复制示例文件
cp .env.example .env

# 编辑配置
vim .env
```

配置内容：
```env
REGISTRY_SERVER=crpi-hoas2aqx96meuhtw.cn-shenzhen.personal.cr.aliyuncs.com
REGISTRY_NAMESPACE=kv-webserver
IMAGE_TAG=latest
```

### 第二步: 构建并推送镜像

```bash
# 方式1: 使用脚本（推荐）
./scripts/push-to-registry.sh

# 方式2: 手动执行
docker-compose build backend
docker tag kv-backend:latest crpi-xxx.com/kv-webserver/kv-backend:latest
docker push crpi-xxx.com/kv-webserver/kv-backend:latest
```

### 第三步: 部署到云服务器

```bash
# 使用自动部署脚本
./scripts/deploy-to-server.sh user@your-server-ip
```

### 第四步: 配置原项目的 nginx

参考 `MIGRATION_GUIDE.md` 文档，修改原项目（my_project）的配置：

1. 修改 `my_project/docker-compose.yml`:
   - 添加 kv 静态文件的 volume 挂载
   - 添加 kv 网络配置

2. 修改 `my_project/nginx/nginx.conf`:
   - 使用 `nginx-unified.conf` 替换原配置

3. 重启原项目:
   ```bash
   cd /opt/my_project
   docker-compose down
   docker-compose up -d
   ```

### 第五步: 验证部署

```bash
# 测试 chatroom 项目
curl http://your-server-ip/

# 测试 kv 项目
curl http://your-server-ip/kv/

# 测试 kv API
curl http://your-server-ip/kv/api/stats
```

---

## 常见问题

### Q1: 推送镜像时提示 "denied: requested access to the resource is denied"

**原因**: 未登录或命名空间不存在

**解决方案**:
```bash
# 登录到阿里云容器镜像服务
docker login crpi-hoas2aqx96meuhtw.cn-shenzhen.personal.cr.aliyuncs.com

# 确认命名空间在阿里云控制台已创建
# 访问: https://cr.console.aliyun.com/
```

### Q2: SSH 连接失败

**解决方案**:
```bash
# 测试 SSH 连接
ssh user@server-ip

# 配置 SSH 免密登录（推荐）
ssh-copy-id user@server-ip
```

### Q3: 服务器上拉取镜像失败

**解决方案**:
```bash
# 在服务器上登录 Docker 仓库
ssh user@server-ip
docker login crpi-hoas2aqx96meuhtw.cn-shenzhen.personal.cr.aliyuncs.com
```

### Q4: 如何更新已部署的服务？

```bash
# 方式1: 使用部署脚本（推荐）
./scripts/push-to-registry.sh v1.0.1
./scripts/deploy-to-server.sh user@server-ip

# 方式2: 在服务器上手动更新
ssh user@server-ip
cd /opt/kv-webserver
docker-compose pull
docker-compose up -d
```

---

## 脚本维护

### 修改镜像仓库

编辑 `.env` 文件：
```env
REGISTRY_SERVER=your-new-registry.com
REGISTRY_NAMESPACE=your-namespace
```

### 修改服务器部署路径

编辑 `deploy-to-server.sh`，修改以下变量：
```bash
PROJECT_DIR="/opt/kv-webserver"      # 项目目录
FRONTEND_DIR="/opt/kv-webserver/frontend"  # 前端文件目录
```

---

## 安全建议

1. **不要将 `.env` 文件提交到 Git**
   - 已在 `.gitignore` 中排除
   - 使用 `.env.example` 作为模板

2. **使用 SSH 密钥认证**
   ```bash
   # 生成 SSH 密钥
   ssh-keygen -t rsa -b 4096

   # 复制到服务器
   ssh-copy-id user@server-ip
   ```

3. **定期更新镜像**
   - 为生产环境使用特定版本标签（如 v1.0.0）
   - 避免在生产环境使用 `latest` 标签

---

## 目录结构

```
kv-webserver/
├── scripts/
│   ├── README.md                 # 本文档
│   ├── push-to-registry.sh       # 镜像推送脚本
│   └── deploy-to-server.sh       # 服务器部署脚本
├── .env                          # 环境变量（不提交到Git）
├── .env.example                  # 环境变量模板
├── docker-compose.yml            # 本地开发配置
├── docker-compose.prod.yml       # 生产环境配置
├── nginx-unified.conf            # 统一nginx配置
├── DEPLOYMENT.md                 # 部署文档
└── MIGRATION_GUIDE.md            # 配置迁移指南
```

---

**需要帮助？** 查看详细文档：
- [DEPLOYMENT.md](../DEPLOYMENT.md) - 完整部署指南
- [MIGRATION_GUIDE.md](../MIGRATION_GUIDE.md) - 原项目配置修改指南
