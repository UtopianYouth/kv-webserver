#!/bin/bash
# 上传前端静态文件到云服务器

set -e

# 切换到项目根目录
cd "$(dirname "$0")/.."

# 输入服务器信息
read -p "服务器IP: " SERVER_IP
read -p "用户名: " USERNAME
read -p "目标目录: " REMOTE_DIR

# 上传前端文件
scp -r frontend/* ${USERNAME}@${SERVER_IP}:${REMOTE_DIR}/

echo "✓ 前端文件上传完成: ${REMOTE_DIR}"
