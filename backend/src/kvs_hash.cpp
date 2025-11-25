#include "kvstore.h"
#include <shared_mutex>

kvs_hash_t global_hash;

// 读写锁
std::shared_mutex global_hash_rwlock;


/**
 *  @return
 *  hash mapping value: success, -1: failed
 */
static int _hash(char* key, int size) {
    if (!key) {
        return -1;
    }

    int sum = 0;
    int i = 0;

    while (key[i] != 0) {
        sum += key[i];
        ++i;
    }

    return sum % size;
}

/*
    @return: not NULL,success; NULL, failed
*/
/**
 *  @return
 *  NOT NULL: success; NULL: failed
 */
hashnode_t* _create_node(char* key, char* value) {

    hashnode_t* node = (hashnode_t*)kvs_malloc(sizeof(hashnode_t));

    if (!node) {
        return NULL;
    }

#if ENABLE_KEY_POINTER
    char* kcopy = (char*)kvs_malloc(strlen(key) + 1);
    if (kcopy == NULL) {
        return NULL;
    }
    memset(kcopy, 0, strlen(key) + 1);
    strncpy(kcopy, key, strlen(key));
    node->key = kcopy;

    char* kvalue = (char*)kvs_malloc(strlen(value) + 1);
    if (kvalue == NULL) {
        return NULL;
    }
    memset(kvalue, 0, strlen(value) + 1);
    strncpy(kvalue, value, strlen(value));
    node->value = kvalue;
#else   
    strcpy(node->key, key, MAX_KEY_LEN);
    strcpy(node->value, value, MAX_VALUE_LEN);
#endif

    node->next = NULL;
    return node;
}

/**
 *  @return
 *  0: success, -1: failed
 */
int kvs_hash_create(kvs_hash_t* hash) {
    if (!hash) {
        return -1;
    }

    hash->nodes = (hashnode_t**)kvs_malloc(sizeof(hashnode_t*) * KVS_HASH_SIZE);

    if (!hash->nodes) {
        return -1;
    }

    // 初始化所有槽位为NULL
    for (int i = 0; i < KVS_HASH_SIZE; i++) {
        hash->nodes[i] = NULL;
    }

    hash->max_slots = KVS_HASH_SIZE;
    hash->count = 0;

    return 0;
}

void kvs_hash_destroy(kvs_hash_t* hash) {
    if (!hash) {
        return;
    }

    int i = 0;

    // free all linklist
    for (i = 0;i < hash->max_slots;++i) {
        hashnode_t* node = hash->nodes[i];

        while (node != NULL) {
            hashnode_t* tmp = node;
            node = node->next;
            hash->nodes[i] = node;

            kvs_free(tmp);
        }
    }

    kvs_free(hash->nodes);
}

// 5 + 2

/*
    @return: exist,1; success,0; error,-1
*/
/**
 *  @return
 *  -1: ERROR, 0: SUCCESS, 1: EXIST
 */
static char* kvs_hash_get_internal(kvs_hash_t* hash, char* key) {
    if (!hash || !key) {
        return NULL;
    }

    int idx = _hash(key, KVS_HASH_SIZE);
    hashnode_t* node = hash->nodes[idx];

    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }

    return NULL;
}

int kvs_hash_set(hashtable_t* hash, char* key, char* value) {
    std::unique_lock<std::shared_mutex> lock(global_hash_rwlock);

    if (!hash || !key || !value) {
        return -1;
    }

    int idx = _hash(key, KVS_HASH_SIZE);      // hash func ==> hash mapping

    hashnode_t* node = hash->nodes[idx];

    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return 1;   // exist
        }
        node = node->next;
    }

    hashnode_t* new_node = _create_node(key, value);
    new_node->next = hash->nodes[idx];
    hash->nodes[idx] = new_node;

    ++hash->count;

    return 0;
}


/**
 *  @return
 *  if NULL: NO EXIST, ELSE: THE VALUE OF KEY
 */
char* kvs_hash_get(kvs_hash_t* hash, char* key) {
    std::shared_lock<std::shared_mutex> lock(global_hash_rwlock);
    return kvs_hash_get_internal(hash, key);
}

/**
 *  @return
 *  -1: ERROR; 0: SUCCESS, 1: NO EXIST
 */
int kvs_hash_mod(kvs_hash_t* hash, char* key, char* value) {
    std::unique_lock<std::shared_mutex> lock(global_hash_rwlock);

    if (!hash || !key) {
        return -1;
    }

    int idx = _hash(key, KVS_HASH_SIZE);

    hashnode_t* node = hash->nodes[idx];

    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            break;      // exist
        }
        node = node->next;
    }

    if (node == NULL) {
        return 1;   // no exist
    }

    // exist
    kvs_free(node->value);
    char* kvalue = (char*)kvs_malloc(strlen(value) + 1);
    if (kvalue == NULL) {
        return -1;
    }
    memset(kvalue, 0, strlen(value) + 1);
    strncpy(kvalue, value, strlen(value));

    node->value = kvalue;

    return 0;
}

/**
 *  @return
 *  -1: ERROR, 0: SUCCESS, 1: NO EXIST
 */
int kvs_hash_del(kvs_hash_t* hash, char* key) {
    std::unique_lock<std::shared_mutex> lock(global_hash_rwlock);

    if (!hash || !key) {
        return -1;
    }

    int idx = _hash(key, KVS_HASH_SIZE);

    hashnode_t* head = hash->nodes[idx];
    if (head == NULL) {
        return 1;      // no exist
    }

    // head node
    if (strcmp(head->key, key) == 0) {
        hashnode_t* tmp = head->next;
        hash->nodes[idx] = tmp;

        kvs_free(head);
        --hash->count;

        return 0;
    }

    // no head node
    hashnode_t* cur = head;
    while (cur->next != NULL) {
        if (strcmp(cur->next->key, key) == 0) {
            break;
        }
        cur = cur->next;
    }

    if (cur->next == NULL) {
        return -1;      // no exist
    }

    hashnode_t* tmp = cur->next;
    cur->next = tmp->next;      // del linklist node, relink
#if ENABLE_KEY_POINTER
    kvs_free(tmp->key);
    kvs_free(tmp->value);
#endif

    kvs_free(tmp);
    --hash->count;

    return 0;
}

/*
    @return: 0,exist; 1, no exist
*/
/**
 *  @return
 *  -1: ERROR, 0: EXIST, 1: NO EXIST
 */
int kvs_hash_exist(kvs_hash_t* hash, char* key) {
    std::shared_lock<std::shared_mutex> lock(global_hash_rwlock);

    if (!hash || !key) {
        return -1;
    }
    char* value = kvs_hash_get_internal(hash, key);
    if (value) {
        return 0;
    }

    return 1;
}