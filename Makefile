# Makefile for KV-WebServer

CC = gcc
CXX = g++
CFLAGS = -Wall -g -Iinclude
CXXFLAGS = -Wall -g -Iinclude -std=c++17
LDFLAGS = -lpthread

TARGET = bin/kv-webserver
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
BUILD_DIR = build

# C++源文件
CXX_SRCS = $(SRC_DIR)/kvs_array.cpp \
           $(SRC_DIR)/kvs_hash.cpp \
           $(SRC_DIR)/kvs_rbtree.cpp \
           $(SRC_DIR)/http_connection.cpp \
           $(SRC_DIR)/lst_timer.cpp \
           $(SRC_DIR)/threadpool.cpp \
           $(SRC_DIR)/http_kvs_connection.cpp \
           $(SRC_DIR)/kvs_handler.cpp \
           $(SRC_DIR)/main.cpp

# 默认目标
all: $(TARGET)

# 创建目录
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 直接编译链接，不保留.o文件
$(TARGET): $(CXX_SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(CXX_SRCS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	rm -f $(SRC_DIR)/*.o

run: $(TARGET)
	./$(TARGET) 8080
