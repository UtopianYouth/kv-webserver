#!/bin/bash
# 推送镜像到阿里云容器镜像仓库

set -e

# 加载环境变量
source .env

# 获取标签（默认latest）
TAG=${1:-${IMAGE_TAG:-latest}}

# 定义镜像名称
LOCAL_IMAGE="kv-backend:latest"
REMOTE_IMAGE="${REGISTRY_SERVER}/${REGISTRY_NAMESPACE}/kv-backend:${TAG}"

# 构建镜像
docker-compose build backend

# 打标签
docker tag $LOCAL_IMAGE $REMOTE_IMAGE

# 推送镜像
docker push $REMOTE_IMAGE

echo "✓ 推送完成: $REMOTE_IMAGE"
