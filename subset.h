#include <stdlib.h>
#include <string.h> 

#define int_b_table_to_list(b_table, table_size) _b_table_to_list(b_table, table_size, sizeof(int))
#define char_b_table_to_list(b_table) _b_table_to_list(b_table, 256, sizeof(unsigned char))
#define SS_union(x, y) _SS_union(&x, y)
#define SS_inv(x) _SS_inv(&x)

typedef struct Subset{
    bool* table;
    int capacity;
    int count;
} Subset;

void* _b_table_to_list(bool* b_table, int table_size, size_t stride);
Subset SS_initialize_empty(int states_length);
Subset SS_initialize(int states_length, int* add_states, int states_amount);
void SS_destroy(Subset* subset);
Subset SS_deep_copy(Subset subset);
void SS_add(Subset* subset, int new_state);
void SS_remove(Subset* subset, int rem_state);
void _SS_union(Subset* subset1, Subset subset2);
void _SS_inv(Subset* subset);
bool SS_equal(Subset subset1, Subset subset2);
bool SS_in(Subset subset1, int state);
bool SS_list_in(Subset* subset_list, Subset elem);
int SS_list_index(Subset* subset_list, Subset elem);
int* SS_to_list_indexes(Subset subset);
void SS_print(Subset subset);