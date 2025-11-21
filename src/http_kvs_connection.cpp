#include <string.h>
#include <stdio.h>
#include "http_kvs_connection.h"


// 外部函数声明
extern void modifyFDEpoll(int epoll_fd, int fd, int event_num);

HttpKvsConnection::HttpKvsConnection() : HttpConnection() {
}

HttpKvsConnection::~HttpKvsConnection() {
}

// JSON解析
bool HttpKvsConnection::parseJsonField(const char* json, const char* field, char* value, int max_len) {
    if (json == NULL || field == NULL || value == NULL) {
        return false;
    }

    // 查找字段名
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", field);

    const char* field_pos = strstr(json, search_pattern);
    if (field_pos == NULL) {
        return false;
    }

    // 查找冒号后的值
    const char* colon_pos = strchr(field_pos, ':');
    if (colon_pos == NULL) {
        return false;
    }

    // 跳过空白字符
    colon_pos++;
    while (*colon_pos == ' ' || *colon_pos == '\t') {
        colon_pos++;
    }

    // 提取值
    if (*colon_pos == '"') {
        // 字符串值
        colon_pos++;
        const char* end_quote = strchr(colon_pos, '"');
        if (end_quote == NULL) {
            return false;
        }
        int len = end_quote - colon_pos;
        if (len >= max_len) {
            len = max_len - 1;
        }
        strncpy(value, colon_pos, len);
        value[len] = '\0';
        return true;
    }
    else {
        // 非字符串值
        int i = 0;
        while (*colon_pos != ',' && *colon_pos != '}' && *colon_pos != '\0' && i < max_len - 1) {
            if (*colon_pos != ' ' && *colon_pos != '\t' && *colon_pos != '\n') {
                value[i++] = *colon_pos;
            }
            colon_pos++;
        }
        value[i] = '\0';
        return i > 0;
    }
}

// 处理kv存储请求
HttpConnection::HTTP_CODE HttpKvsConnection::processKvsRequest() {
    // 从请求体中解析JSON
    char cmd[32] = { 0 };
    char key[256] = { 0 };
    char value[512] = { 0 };

    // 假设请求体在m_read_buf的末尾部分
    char* json_body = m_read_buf + m_checked_index;

    // 解析JSON字段
    if (!parseJsonField(json_body, "cmd", cmd, sizeof(cmd))) {
        return BAD_REQUEST;
    }

    if (!parseJsonField(json_body, "key", key, sizeof(key))) {
        return BAD_REQUEST;
    }

    // value是可选的
    parseJsonField(json_body, "value", value, sizeof(value));

    // 调用kv存储处理函数
    char response_json[4096] = { 0 };
    int json_len = kvs_handle_command(cmd, key,
        value[0] != '\0' ? value : NULL, response_json);

    if (json_len <= 0) {
        return INTERNAL_ERROR;
    }

    // 将JSON响应写入写缓冲区
    return writeJsonResponse(response_json) ? GET_REQUEST : INTERNAL_ERROR;
}

// 生成JSON响应
bool HttpKvsConnection::writeJsonResponse(const char* json_content) {
    if (json_content == NULL) {
        return false;
    }

    int content_len = strlen(json_content);

    // 添加响应状态行
    addStatusLine(200, "OK");

    // 添加响应头
    addResponse("Content-Type: application/json\r\n");
    addResponse("Access-Control-Allow-Origin: *\r\n");  // CORS支持
    addResponse("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
    addResponse("Access-Control-Allow-Headers: Content-Type\r\n");
    addContentLength(content_len);
    addKeepAlive();
    addBlankLine();

    // 添加响应体
    addContent(json_content);

    // 设置分散写对象
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_index;
    m_iv_count = 1;

    bytes_to_send = m_write_index;
    return true;
}

// 重写process方法
void HttpKvsConnection::process() {
    // 解析HTTP请求
    HTTP_CODE read_ret = processRead();

    if (read_ret == NO_REQUEST) {
        // 需要继续读取客户端请求的内容
        modifyFDEpoll(m_epoll_fd, m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = false;

    if (m_method == POST && m_url != NULL && strcmp(m_url, "/api/kv") == 0) {
        // POST: /api/kv
        read_ret = processKvsRequest();
        write_ret = (read_ret == GET_REQUEST);
    }
    else if (m_method == GET && m_url != NULL && strcmp(m_url, "/api/stats") == 0) {
        // GET: /api/stats
        char stats_json[2048] = { 0 };
        kvs_get_stats(stats_json);
        write_ret = writeJsonResponse(stats_json);
    }
    else {
        // 静态文件
        write_ret = processWrite(read_ret);
    }

    if (!write_ret) {
        closeConnection();
        return;
    }

    // 监测文件描述符写事件
    modifyFDEpoll(m_epoll_fd, m_sockfd, EPOLLOUT);
}
