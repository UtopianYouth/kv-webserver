#!/bin/bash

# KV存储系统 Docker 快速部署脚本
# 使用方法: ./deploy.sh [build|start|stop|restart|logs|clean]

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目配置
PROJECT_NAME="kv-webserver"
IMAGE_NAME="kv-webserver:latest"
CONTAINER_NAME="kv-webserver"
PORT=8080

# 打印带颜色的信息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查Docker是否安装
check_docker() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker未安装，请先安装Docker"
        exit 1
    fi
    print_success "Docker已安装: $(docker --version)"
}

# 检查端口是否被占用
check_port() {
    if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_warning "端口 $PORT 已被占用，是否继续？(y/n)"
        read -r response
        if [[ ! "$response" =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# 构建镜像
build_image() {
    print_info "开始构建Docker镜像..."

    if docker build -t $IMAGE_NAME .; then
        print_success "镜像构建成功"
        docker images | grep $PROJECT_NAME
    else
        print_error "镜像构建失败"
        exit 1
    fi
}

# 启动容器
start_container() {
    print_info "启动容器..."

    # 检查容器是否已存在
    if docker ps -a | grep -q $CONTAINER_NAME; then
        print_warning "容器已存在，正在删除旧容器..."
        docker rm -f $CONTAINER_NAME
    fi

    # 启动新容器
    docker run -d \
        --name $CONTAINER_NAME \
        -p $PORT:$PORT \
        --restart unless-stopped \
        $IMAGE_NAME

    if [ $? -eq 0 ]; then
        print_success "容器启动成功"
        print_info "访问地址: http://localhost:$PORT"

        # 等待容器启动
        sleep 2

        # 显示容器状态
        docker ps | grep $CONTAINER_NAME

        # 测试服务
        if curl -s http://localhost:$PORT > /dev/null; then
            print_success "服务运行正常"
        else
            print_warning "服务可能尚未完全启动，请稍后访问"
        fi
    else
        print_error "容器启动失败"
        exit 1
    fi
}

# 停止容器
stop_container() {
    print_info "停止容器..."

    if docker ps | grep -q $CONTAINER_NAME; then
        docker stop $CONTAINER_NAME
        print_success "容器已停止"
    else
        print_warning "容器未运行"
    fi
}

# 重启容器
restart_container() {
    print_info "重启容器..."
    stop_container

    if docker ps -a | grep -q $CONTAINER_NAME; then
        docker start $CONTAINER_NAME
        print_success "容器已重启"
        print_info "访问地址: http://localhost:$PORT"
    else
        print_warning "容器不存在，正在创建新容器..."
        start_container
    fi
}

# 查看日志
view_logs() {
    print_info "查看容器日志..."

    if docker ps -a | grep -q $CONTAINER_NAME; then
        docker logs -f $CONTAINER_NAME
    else
        print_error "容器不存在"
        exit 1
    fi
}

# 清理资源
clean_resources() {
    print_warning "清理所有资源（容器、镜像）..."
    echo "这将删除容器和镜像，是否继续？(y/n)"
    read -r response

    if [[ "$response" =~ ^[Yy]$ ]]; then
        # 停止并删除容器
        if docker ps -a | grep -q $CONTAINER_NAME; then
            print_info "删除容器..."
            docker rm -f $CONTAINER_NAME
        fi

        # 删除镜像
        if docker images | grep -q $PROJECT_NAME; then
            print_info "删除镜像..."
            docker rmi $IMAGE_NAME
        fi

        print_success "清理完成"
    else
        print_info "取消清理"
    fi
}

# 显示状态
show_status() {
    print_info "容器状态:"
    docker ps -a | grep $CONTAINER_NAME || print_warning "容器不存在"

    echo ""
    print_info "镜像信息:"
    docker images | grep $PROJECT_NAME || print_warning "镜像不存在"

    echo ""
    if docker ps | grep -q $CONTAINER_NAME; then
        print_info "资源使用情况:"
        docker stats --no-stream $CONTAINER_NAME
    fi
}

# 完整部署（构建+启动）
full_deploy() {
    print_info "开始完整部署流程..."
    check_docker
    check_port
    build_image
    start_container
    show_status
    print_success "部署完成！"
}

# 使用Docker Compose部署
deploy_with_compose() {
    print_info "使用Docker Compose部署..."

    if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
        print_error "Docker Compose未安装"
        exit 1
    fi

    # 检查是否使用docker compose或docker-compose命令
    if docker compose version &> /dev/null; then
        COMPOSE_CMD="docker compose"
    else
        COMPOSE_CMD="docker-compose"
    fi

    $COMPOSE_CMD up -d --build
    print_success "Docker Compose部署成功"
    print_info "访问地址: http://localhost:$PORT"
}

# 显示帮助信息
show_help() {
    echo "KV存储系统 Docker 部署脚本"
    echo ""
    echo "使用方法:"
    echo "  ./deploy.sh [命令]"
    echo ""
    echo "可用命令:"
    echo "  deploy        - 完整部署（构建镜像+启动容器）"
    echo "  compose       - 使用Docker Compose部署"
    echo "  build         - 构建Docker镜像"
    echo "  start         - 启动容器"
    echo "  stop          - 停止容器"
    echo "  restart       - 重启容器"
    echo "  logs          - 查看容器日志"
    echo "  status        - 显示容器和镜像状态"
    echo "  clean         - 清理容器和镜像"
    echo "  help          - 显示帮助信息"
    echo ""
    echo "示例:"
    echo "  ./deploy.sh deploy    # 完整部署"
    echo "  ./deploy.sh logs      # 查看日志"
    echo "  ./deploy.sh restart   # 重启服务"
}

# 主函数
main() {
    case "$1" in
        deploy)
            full_deploy
            ;;
        compose)
            deploy_with_compose
            ;;
        build)
            check_docker
            build_image
            ;;
        start)
            check_docker
            check_port
            start_container
            ;;
        stop)
            stop_container
            ;;
        restart)
            restart_container
            ;;
        logs)
            view_logs
            ;;
        status)
            show_status
            ;;
        clean)
            clean_resources
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            print_error "未知命令: $1"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

# 检查是否提供了参数
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

# 执行主函数
main "$@"
