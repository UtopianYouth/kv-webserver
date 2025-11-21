#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shared_mutex>
#include "kvs_handler.h"


void* kvs_malloc(size_t size) {
    return malloc(size);
}

void kvs_free(void* ptr) {
    free(ptr);
}

// kv cmd
enum {
    KVS_CMD_START = 0,

    // array
    KVS_CMD_SET = KVS_CMD_START,
    KVS_CMD_GET,
    KVS_CMD_DEL,
    KVS_CMD_MOD,
    KVS_CMD_EXIST,

    // rbtree
    KVS_CMD_RSET,
    KVS_CMD_RGET,
    KVS_CMD_RDEL,
    KVS_CMD_RMOD,
    KVS_CMD_REXIST,

    // hash
    KVS_CMD_HSET,
    KVS_CMD_HGET,
    KVS_CMD_HDEL,
    KVS_CMD_HMOD,
    KVS_CMD_HEXIST,

    KVS_CMD_COUNT,
};

// 命令字符串
const char* command[] = {
    "SET", "GET", "DEL", "MOD", "EXIST",
    "RSET", "RGET", "RDEL", "RMOD", "REXIST",
    "HSET", "HGET", "HDEL", "HMOD", "HEXIST"
};

// init kvstore
int init_kvengine(void) {
#if ENABLE_ARRAY
    memset(&global_array, 0, sizeof(kvs_array_t));
    if (-1 == kvs_array_create(&global_array)) {
        return -1;
    }
#endif

#if ENABLE_RBTREE
    memset(&global_rbtree, 0, sizeof(kvs_rbtree_t));
    if (-1 == kvs_rbtree_create(&global_rbtree)) {
        return -1;
    }
#endif

#if ENABLE_HASH
    memset(&global_hash, 0, sizeof(kvs_hash_t));
    if (-1 == kvs_hash_create(&global_hash)) {
        return -1;
    }
#endif

    return 0;
}

// destroy kvstore
void destroy_kvengine(void) {
#if ENABLE_ARRAY
    kvs_array_destroy(&global_array);
#endif

#if ENABLE_RBTREE
    kvs_rbtree_destroy(&global_rbtree);
#endif

#if ENABLE_HASH
    kvs_hash_destroy(&global_hash);
#endif
}

// get kv statistical info
int kvs_get_stats(char* response) {
    if (response == NULL) {
        return -1;
    }

    int array_count = 0, array_max = KVS_ARRAY_SIZE;
    int hash_count = 0, hash_max = KVS_HASH_SIZE;
    int rbtree_count = 0, rbtree_max = KVS_RBTREE_SIZE;

#if ENABLE_ARRAY
    {
        std::shared_lock<std::shared_mutex> lock(global_array_rwlock);
        array_count = global_array.total;
    }
#endif

#if ENABLE_HASH
    {
        std::shared_lock<std::shared_mutex> lock(global_hash_rwlock);
        hash_count = global_hash.count;
    }
#endif

#if ENABLE_RBTREE
    {
        std::shared_lock<std::shared_mutex> lock(global_rbtree_rwlock);
        rbtree_count = global_rbtree.count;
    }
#endif

    return sprintf(response,
        "{\"status\":\"OK\",\"data\":{"
        "\"array\":{\"count\":%d,\"max\":%d,\"remaining\":%d},"
        "\"hash\":{\"count\":%d,\"max\":%d,\"remaining\":%d},"
        "\"rbtree\":{\"count\":%d,\"max\":%d,\"remaining\":%d}"
        "}}",
        array_count, array_max, array_max - array_count,
        hash_count, hash_max, hash_max - hash_count,
        rbtree_count, rbtree_max, rbtree_max - rbtree_count
    );
}

/**
 * cmd: SET/GET/DEL/MOD/EXIST/RSET/RGET/HSET/HGET...
 * key: [value](GET/DEL/EXIST haven't value)
 * response: json type
 * @return the size of response str
 */
int kvs_handle_command(const char* cmd, const char* key, const char* value, char* response) {
    if (cmd == NULL || key == NULL || response == NULL) {
        return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Invalid parameters\"}");
    }

    // 查找命令类型
    int cmd_type = KVS_CMD_START;
    for (cmd_type = KVS_CMD_START; cmd_type < KVS_CMD_COUNT; ++cmd_type) {
        if (strcmp(cmd, command[cmd_type]) == 0) {
            break;
        }
    }

    if (cmd_type >= KVS_CMD_COUNT) {
        return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Unknown command\"}");
    }

    int ret = 0;
    char* result = NULL;
    char data_json[2048] = { 0 };

    switch (cmd_type) {
#if ENABLE_ARRAY
        // Array
    case KVS_CMD_SET:
        if (value == NULL) {
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Value required\"}");
        }
        ret = kvs_array_set(&global_array, (char*)key, (char*)value);
        if (ret == -1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to set\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Set successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"EXIST\",\"message\":\"Key already exists\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 2) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"FULL\",\"message\":\"Array storage full\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_GET:
        result = kvs_array_get(&global_array, (char*)key);
        if (result == NULL) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"%s\",\"data\":%s}",
                result, strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_DEL:
        ret = kvs_array_del(&global_array, (char*)key);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Deleted successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to delete\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_MOD:
        if (value == NULL) {
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Value required\"}");
        }
        ret = kvs_array_mod(&global_array, (char*)key, (char*)value);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Modified successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to modify\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_EXIST:
        ret = kvs_array_exist(&global_array, (char*)key);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"EXIST\",\"message\":\"Key exists\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to check\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;
#endif

#if ENABLE_RBTREE
        // RBTree
    case KVS_CMD_RSET:
        if (value == NULL) {
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Value required\"}");
        }
        ret = kvs_rbtree_set(&global_rbtree, (char*)key, (char*)value);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Set successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"EXIST\",\"message\":\"Key already exists\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to set\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_RGET:
        result = kvs_rbtree_get(&global_rbtree, (char*)key);
        if (result == NULL) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"%s\",\"data\":%s}",
                result, strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_RDEL:
        ret = kvs_rbtree_del(&global_rbtree, (char*)key);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Deleted successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to delete\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_RMOD:
        if (value == NULL) {
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Value required\"}");
        }
        ret = kvs_rbtree_mod(&global_rbtree, (char*)key, (char*)value);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Modified successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to modify\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_REXIST:
        ret = kvs_rbtree_exist(&global_rbtree, (char*)key);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"EXIST\",\"message\":\"Key exists\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to check\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;
#endif

#if ENABLE_HASH
        // Hash
    case KVS_CMD_HSET:
        if (value == NULL) {
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Value required\"}");
        }
        ret = kvs_hash_set(&global_hash, (char*)key, (char*)value);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Set successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"EXIST\",\"message\":\"Key already exists\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to set\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_HGET:
        result = kvs_hash_get(&global_hash, (char*)key);
        if (result == NULL) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"%s\",\"data\":%s}",
                result, strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_HDEL:
        ret = kvs_hash_del(&global_hash, (char*)key);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Deleted successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to delete\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_HMOD:
        if (value == NULL) {
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Value required\"}");
        }
        ret = kvs_hash_mod(&global_hash, (char*)key, (char*)value);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"OK\",\"message\":\"Modified successfully\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to modify\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;

    case KVS_CMD_HEXIST:
        ret = kvs_hash_exist(&global_hash, (char*)key);
        if (ret == 0) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"EXIST\",\"message\":\"Key exists\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else if (ret == 1) {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"NO_EXIST\",\"message\":\"Key not found\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        else {
            kvs_get_stats(data_json);
            return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Failed to check\",\"data\":%s}",
                strchr(data_json, '{'));
        }
        break;
#endif
    default:
        return sprintf(response, "{\"status\":\"ERROR\",\"message\":\"Unsupported command\"}");
    }

    return 0;
}
