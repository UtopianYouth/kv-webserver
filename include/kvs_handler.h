#ifndef __KVS_HANDLER_H__
#define __KVS_HANDLER_H__

#include "kvstore.h"
#include <shared_mutex>

// 全局KV存储实例
#if ENABLE_ARRAY
extern kvs_array_t global_array;
extern std::shared_mutex global_array_rwlock;
#endif

#if ENABLE_RBTREE
extern kvs_rbtree_t global_rbtree;
extern std::shared_mutex global_rbtree_rwlock;
#endif

#if ENABLE_HASH
extern kvs_hash_t global_hash;
extern std::shared_mutex global_hash_rwlock;
#endif

// 函数声明
int init_kvengine(void);

void destroy_kvengine(void);

/**
 * cmd: SET/GET/DEL/MOD/EXIST/RSET/RGET/HSET/HGET...
 * key: [value](GET/DEL/EXIST haven't value)
 * response: json type
 * @return the size of response str
 */
int kvs_handle_command(const char* cmd, const char* key, const char* value, char* response);

// get statistics of kvs info
int kvs_get_stats(char* response);

#endif
