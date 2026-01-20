#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "dynarray.h"
#include "hash.h"


Hash _hash_create(int node_amount, size_t stride, void* ptr_func)
{
    Hash hash;
    hash.table = calloc(node_amount, sizeof(Node*));
    hash.obj_storage = _dynarray_create(DYNARRAY_DEFAULT_CAP, stride);
    hash.stride = stride;
    hash.f_ptr = ptr_func;
    hash.capacity = node_amount;
    hash.holes = dynarray_create(int);
    hash.count = 0;

    return hash;
}


bool _hash_in(Hash *hash, void *xptr, bool (*f_equality_ptr)(void*, void*), int action){
    uint64_t hash_id = hash->f_ptr(xptr);

    int bucket_id = hash_id%(hash->capacity);
    
    Node* slot = hash->table[bucket_id];
    if(slot != NULL){
        Node* tmp = slot;
        Node* prev = NULL;

        while(tmp != NULL){
            uint64_t curr_hash_id = hash->f_ptr((char*) hash->obj_storage + tmp->storage_index * hash->stride);
            if(hash_id == curr_hash_id && f_equality_ptr(xptr, (char*) hash->obj_storage + tmp->storage_index * hash->stride)){
                
                if(action == -1){
                    if(prev != NULL){
                    prev->next = tmp->next;
                    }
                    else{
                        hash->table[bucket_id] = tmp->next;
                    }
                    dynarray_push(hash->holes,tmp->storage_index);
                    hash->count --;
                    free(tmp);
                }

                return true;
                
            }
            prev = tmp;
            tmp = tmp->next;
        }
    }

    if(action == 1){
        Node* node = malloc(sizeof(Node));
        node->next = NULL;
        int placement_index = dynarray_length(hash->obj_storage);
        if(dynarray_length(hash->holes)>0){
            dynarray_pop(hash->holes, &placement_index);
            _dynarray_replace(hash->obj_storage, xptr, placement_index);
        }
        else{
            hash->obj_storage = _dynarray_push(hash->obj_storage, xptr);
        }
        
        node->storage_index = placement_index;

        if(slot != NULL){
            node->next = slot;
        }

        hash->table[bucket_id] = node;

        hash->count ++;
    }

    return false;
}

void *_hash_to_list(Hash *hash){
    void* arr = _dynarray_create(hash->count, hash->stride);
    for(int i=0;i<hash->capacity;i++){
        if(hash->table[i] != NULL){

            Node* tmp = hash->table[i];

            while(tmp != NULL){
                arr = _dynarray_push(arr, (char*) hash->obj_storage + tmp->storage_index * hash->stride);
                tmp = tmp->next;
            }
        }
    }
    return arr;
}

uint64_t _hash_int(void *xptr) {
    uint64_t x = (uint64_t) *((int*) xptr);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

uint64_t hash_combine( uint64_t lhs, uint64_t rhs ) {
    if (sizeof(uint64_t) >= 8) {
        lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
    } 
    else {
        lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    }
    return lhs;
}