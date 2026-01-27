#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "dynarray.h"
#include "subset.h"

void* _b_table_to_list(bool* b_table, int table_size, size_t stride){
    void* sub_list = _dynarray_create(DYNARRAY_DEFAULT_CAP, stride);
    for(int i = 0; i < table_size;i++){
        if(b_table[i] == true){
            size_t tmp = (size_t)i;
            dynarray_push(sub_list, tmp);
        }
    }
    return sub_list;
}

Subset SS_initialize_empty(int cap){
    Subset subset;
    subset.capacity = cap;
    subset.count = 0;
    subset.table = malloc(cap * sizeof(bool));
    memset(subset.table, 0, cap * sizeof(bool));

    return subset;
}

Subset SS_initialize(int cap, int* add, int add_amount){
    Subset subset;
    subset.capacity = cap;
    subset.count = 0;
    subset.table = malloc(cap * sizeof(bool));
    memset(subset.table, 0, cap * sizeof(bool));

    for(int i = 0;i<add_amount;i++){
        assert(add[i] < cap);
        if(subset.table[add[i]] == false){
            subset.table[add[i]] = true;
            subset.count += 1;
        }  
    }

    return subset;
}

void SS_destroy(Subset* subset){
    free(subset->table);
}

Subset SS_deep_copy(Subset subset){
    Subset copy = SS_initialize_empty(subset.capacity);
    copy.count = subset.count;
    for(int i = 0;i<subset.capacity;i++){
        if(subset.table[i] == true){
            copy.table[i] = true;
        }
    }

    return copy;
}

void SS_add(Subset* subset, int new_state){
    assert(new_state < subset->capacity);
    if(subset->table[new_state] == false){
            subset->table[new_state] = true;
            subset->count += 1;
    }  
    subset->table[new_state] = true;
}

void SS_remove(Subset* subset, int rem_state){
    assert(rem_state < subset->capacity);
    if(subset->table[rem_state] == true){
            subset->table[rem_state] = false;
            subset->count -= 1;
    }  
}

void _SS_union(Subset* subset1, Subset subset2){
    assert(subset1->capacity == subset2.capacity);
    for(int i = 0;i<subset1->capacity;i++){
        if(subset1->table[i] == false && subset2.table[i] == true){
            subset1->count ++;
            subset1->table[i] = true;
        }
    }
}

void _SS_inv(Subset* subset){
    for(int i = 0;i<subset->capacity;i++){
        subset->table[i] = !subset->table[i];
    }

    subset->count = subset->capacity - subset->count;
}

bool SS_equal(Subset subset1, Subset subset2){
    assert(subset1.capacity == subset2.capacity);
    for(int i = 0;i<subset1.capacity;i++){
        if(subset1.table[i] != subset2.table[i]){
            return false;
        }
    }
    return true;
}

bool SS_in(Subset subset1, int state){
    assert(state < subset1.capacity);
    return subset1.table[state];
}

bool SS_list_in(Subset* subset_list, Subset elem){
    for(int i = 0;i<dynarray_length(subset_list);i++){
        if(SS_equal(subset_list[i], elem)){
            return true;
        }
    }

    return false;
}

int SS_list_index(Subset* subset_list, Subset elem){
    for(int i = 0;i<dynarray_length(subset_list);i++){
        if(SS_equal(subset_list[i], elem)){
            return i;
        }
    }

    return -1;
}

int* SS_to_list_indexes(Subset subset){
    return int_b_table_to_list(subset.table, subset.capacity);
}

void SS_print(Subset subset){
    printf("Subset\n");
    printf("Capacity: %d\n", subset.capacity);
    printf("Length: %d\n", subset.count);
    for(int i = 0; i < subset.capacity;i++){
        if(subset.table[i] == true){
            printf("%d, ", i);
        } 
    }
    printf("\n");
}
