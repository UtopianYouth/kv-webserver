#ifndef HTTP_KVS_CONNECTION_H
#define HTTP_KVS_CONNECTION_H

#include "http_connection.h"
#include "kvs_handler.h"

// 扩展HttpConnection类以支持POST请求和JSON
class HttpKvsConnection : public HttpConnection {
public:
    HttpKvsConnection();
    ~HttpKvsConnection();

    // 重写process方法以支持KV存储
    void process();

private:
    // JSON解析
    bool parseJsonField(const char* json, const char* field, char* value, int max_len);

    // 处理kv存储请求
    HTTP_CODE processKvsRequest();

    // 生成JSON响应
    bool writeJsonResponse(const char* json_content);
};

#endif
