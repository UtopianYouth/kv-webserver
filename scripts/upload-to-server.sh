#!/bin/bash
# 上传配置文件到云服务器

set -e

# 切换到项目根目录
cd "$(dirname "$0")/.."

# 输入服务器信息
read -p "服务器IP: " SERVER_IP
read -p "用户名: " USERNAME
read -p "目标目录: " REMOTE_DIR

# 创建远程目录
ssh ${USERNAME}@${SERVER_IP} "mkdir -p ${REMOTE_DIR}"

# 上传文件
scp .env ${USERNAME}@${SERVER_IP}:${REMOTE_DIR}/
scp docker-compose.prod.yml ${USERNAME}@${SERVER_IP}:${REMOTE_DIR}/docker-compose.yml

echo "✓ 上传完成: ${USERNAME}@${SERVER_IP}:${REMOTE_DIR}"
