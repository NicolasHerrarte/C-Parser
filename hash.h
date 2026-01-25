#include <stdlib.h>
#include <string.h>  
#include <stdint.h>

#define hash_create(node_amount, type, ptr_func) _hash_create(node_amount, sizeof(type), sizeof(type), ptr_func, false)
#define hash_in(hash, xptr, func) _hash_in(&hash, &xptr, &xptr, func, 0, false)
#define hash_add(hash, xptr, func) _hash_in(&hash, &xptr, &xptr, func, 1, false)
#define hash_remove(hash, xptr, func) _hash_in(&hash, &xptr, &xptr, func, -1, false)
#define hash_get(hash, xptr, func) _hash_get(&hash, &xptr, func, false)

#define dynadict_create(node_amount, type) _hash_create(node_amount, sizeof(type), sizeof(char*), _djb33_hash, true)
#define dynadict_key_in(hash, xptr) _hash_in(&hash, &xptr, &xptr, string_equal, 0, true)
#define dynadict_add(hash, xptr, stptr) _hash_in(&hash, &xptr, &stptr, string_equal, 1, true)
#define dynadict_remove(hash, xptr) _hash_in(&hash, &xptr, &xptr, string_equal, -1, true)
#define dynadict_get(hash, xptr) _hash_get(&hash, &xptr, string_equal, true)

#define hash_to_list(hash) _hash_to_list(&hash)
#define hash_int(num) _hash_int(&num)
#define str_hash(str) _djb33_hash(&str)

typedef struct Node{
    int storage_index;
    uint64_t hash_index;
    struct Node* next;
} Node;

typedef struct Hash{
    int capacity;
    int count;
    size_t stride;
    size_t key_stride;
    uint64_t (*f_ptr)(void*);
    Node** table;
    void* obj_storage;
    void* key_storage;
    int* holes;
} Hash;

Hash _hash_create(int node_amount, size_t stride, size_t key_stride, void* ptr_func, bool uses_key_storage);
bool _hash_in(Hash *hash, void *xptr, void *str_ptr, bool (*f_equality_ptr)(void*, void*), int action, bool uses_key_storage);
void *_hash_get(Hash *hash, void *xptr, bool (*f_equality_ptr)(void*, void*), bool uses_key_storage);
void *_hash_to_list(Hash *hash);
uint64_t _hash_int(void *xptr);
uint32_t _djb33_hash(void *xptr);
uint64_t hash_combine( uint64_t lhs, uint64_t rhs );
bool string_equal(void* ptr1, void* ptr2);