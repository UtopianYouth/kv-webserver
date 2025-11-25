#include "kvstore.h"
#include <shared_mutex>

// singleton
kvs_array_t global_array = { 0 };

// 读写锁
std::shared_mutex global_array_rwlock;

/*
    @return
    -1: falied, 0: success
*/
int kvs_array_create(kvs_array_t* inst) {
    if (!inst) {
        return -1;
    }

    if (inst->table) {
        printf("table has alloc\n");
        return -1;
    }

    inst->table = (kvs_array_item_t*)kvs_malloc(KVS_ARRAY_SIZE * sizeof(kvs_array_item_t));

    if (!inst->table) {
        return -1;
    }

    inst->total = 0;

    return 0;
}

/*
    @return
    - NULL
    - inst->table[i].value
*/
static char* kvs_array_get_internal(kvs_array_t* inst, char* key) {
    if (inst == NULL || key == NULL) {
        return NULL;
    }

    for (int i = 0; i < KVS_ARRAY_SIZE; ++i) {
        if (inst->table[i].key != NULL && strcmp(inst->table[i].key, key) == 0) {
            return inst->table[i].value;
        }
    }

    return NULL;
}

/*
    @return
    -1: ERROR, 0: OK, 1: EXIST, 2: kv_mem full
*/
int kvs_array_set(kvs_array_t* inst, char* key, char* value) {
    std::unique_lock<std::shared_mutex> lock(global_array_rwlock);

    if (inst == NULL || key == NULL || value == NULL) {
        return -1;
    }

    if (inst->total == KVS_ARRAY_SIZE) {
        // item enough
        return -1;
    }

    char* str = kvs_array_get_internal(inst, key);

    if (str) {
        return 1;   // exist
    }

    char* kcopy = (char*)kvs_malloc(strlen(key) + 1);  // heap memory
    if (kcopy == NULL) {
        kvs_free(kcopy);
        return -1;
    }
    memset(kcopy, 0, strlen(key) + 1);
    strncpy(kcopy, key, strlen(key));

    char* kvalue = (char*)kvs_malloc(strlen(value) + 1);   // heap memory
    if (kvalue == NULL) {
        return -1;
    }

    memset(kvalue, 0, strlen(value) + 1);
    strncpy(kvalue, value, strlen(value));

    int i = 0;

    for (i = 0;i < KVS_ARRAY_SIZE;++i) {
        if (inst->table[i].key == NULL) {
            inst->table[i].key = kcopy;
            inst->table[i].value = kvalue;
            inst->total++;
            return 0;
        }
    }

    kvs_free(kcopy);
    kvs_free(kvalue);

    return 2;   // mem full
}

/*
    @return
    if NULL: NO EXIST, else: THE VALUE OF KEY
*/
char* kvs_array_get(kvs_array_t* inst, char* key) {
    std::shared_lock<std::shared_mutex> lock(global_array_rwlock);
    return kvs_array_get_internal(inst, key);
}


/*
    @return
    -1: ERROR, 0: OK, 1: NO EXIST
*/
int kvs_array_mod(kvs_array_t* inst, char* key, char* value) {
    std::unique_lock<std::shared_mutex> lock(global_array_rwlock);

    if (inst == NULL || key == NULL || value == NULL) {
        return -1;
    }

    int i = 0;

    for (i = 0;i < KVS_ARRAY_SIZE;++i) {
        if (inst->table[i].key == NULL) {
            continue;
        }

        if (strcmp(inst->table[i].key, key) == 0) {
            kvs_free(inst->table[i].value);

            char* kvalue = (char*)kvs_malloc(strlen(value) + 1);
            if (kvalue == NULL) {
                return -2;
            }
            memset(kvalue, 0, strlen(value) + 1);
            strncpy(kvalue, value, strlen(value));

            inst->table[i].value = kvalue;

            return 0;
        }
    }

    return 1;       // 1: no exist
}


/*
    @return
    -1: ERROR, 0: OK, 1: NO EXIST
*/
int kvs_array_del(kvs_array_t* inst, char* key) {
    std::unique_lock<std::shared_mutex> lock(global_array_rwlock);

    if (inst == NULL || key == NULL) {
        return -1;
    }

    int i = 0;
    for (i = 0;i < KVS_ARRAY_SIZE;++i) {
        if (inst->table[i].key == NULL) {
            continue;
        }
        if (strcmp(inst->table[i].key, key) == 0) {
            kvs_free(inst->table[i].key);
            inst->table[i].key = NULL;

            kvs_free(inst->table[i].value);
            inst->table[i].value = NULL;

            inst->total--;

            return 0;
        }
    }

    return 1;   // 1: no exist
}

/**
 * @return
 * -1: ERROR, 0: EXIST, 1: NO EXIST
 */
int kvs_array_exist(kvs_array_t* inst, char* key) {
    std::shared_lock<std::shared_mutex> lock(global_array_rwlock);

    if (!inst || !key) {
        return -1;
    }

    char* str = kvs_array_get_internal(inst, key);

    if (str) {
        return 0;
    }

    return 1;
}

void kvs_array_destroy(kvs_array_t* inst) {
    if (!inst) {
        return;
    }

    if (inst->table) {
        kvs_free(inst->table);
    }

    return;
}
