#include <stdlib.h>
#include <string.h>  
#include <stdint.h>

#define hash_create(node_amount, type, ptr_func) _hash_create(node_amount, sizeof(type), ptr_func)
#define hash_in(hash, xptr, func) _hash_in(&hash, &xptr, func, 0)
#define hash_add(hash, xptr, func) _hash_in(&hash, &xptr, func, 1)
#define hash_remove(hash, xptr, func) _hash_in(&hash, &xptr, func, -1)
#define hash_to_list(hash) _hash_to_list(&hash)
#define hash_int(num) _hash_int(&num)

typedef struct Node{
    int storage_index;
    struct Node* next;
} Node;

typedef struct Hash{
    int capacity;
    int count;
    size_t stride;
    uint64_t (*f_ptr)(void*);
    Node** table;
    void* obj_storage;
    int* holes;
} Hash;

Hash _hash_create(int node_amount, size_t stride, void* ptr_func);
bool _hash_in(Hash *hash, void *xptr, bool (*f_equality_ptr)(void*, void*), int action);
void *_hash_to_list(Hash *hash);
uint64_t _hash_int(void *xptr);
uint64_t hash_combine( uint64_t lhs, uint64_t rhs );