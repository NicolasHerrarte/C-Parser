#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "dynarray.h"
#include "subset.h"
#include "hash.h"


enum {
    END,
    EPSILON,
    GOAL,
    LIST,
    PAIR, 
    LEFT_PAR,
    RIGHT_PAR
};

typedef struct Production{
    int alpha;
    int* beta;
} Production;

typedef struct Item{
    int alpha;
    int* beta;
    int lookahead;
    int k;
} Item;

typedef struct CC_Item{
    Item* cc;
    bool marked;
} CC_Item;

typedef struct Grammar{
    int* T;
    int* NT;
    int S;
    Production* productions;
} Grammar;

bool item_equal(Item item1, Item item2){
    if(item1.alpha != item2.alpha){
        return false;
    }
    if(item1.lookahead != item2.lookahead){
        return false;
    }
    if(item1.k != item2.k){
        return false;
    }
    if(dynarray_length(item1.beta) != dynarray_length(item2.beta)){
        return false;
    }
    for(int i = 0;i<dynarray_length(item1.beta);i++){
        if(item1.beta[i] != item2.beta[i]){
            return false;
        }
    }

    return true;
}

bool item_in(Item* items, Item find_item){
    for(int i = 0;i<dynarray_length(items);i++){
        if(item_equal(items[i], find_item)){
            return true;
        }
    }
    return false;
}

Item item_copy(Item original){
    Item new_item;
    new_item.alpha = original.alpha;
    new_item.beta = dynarray_copy(original.beta);
    new_item.lookahead = original.lookahead;
    new_item.k = original.k;

    return new_item;
}

uint64_t hash_item(void* item_ptr){
    Item item = *((Item*) item_ptr);
    uint64_t curr_hash_int = hash_int(item.alpha);
    for(int i=0;i<dynarray_length(item.beta);i++){
        uint64_t tmp_hash_beta = hash_int(item.beta[i]);
        curr_hash_int = hash_combine(curr_hash_int, tmp_hash_beta);
    }
    uint64_t tmp_hash_k = hash_int(item.k);
    uint64_t tmp_hash_look = hash_int(item.lookahead);

    curr_hash_int = hash_combine(curr_hash_int, tmp_hash_k);
    curr_hash_int = hash_combine(curr_hash_int, tmp_hash_look);

    return curr_hash_int;
}

bool hash_item_equal(void* a, void* b) {
    Item* item_a = (Item*)a;
    Item* item_b = (Item*)b;

    if (item_a->alpha != item_b->alpha) return false;
    if (item_a->lookahead != item_b->lookahead) return false;
    if (item_a->k != item_b->k) return false;

    int len_a = dynarray_length(item_a->beta);
    int len_b = dynarray_length(item_b->beta);

    if (len_a != len_b) return false;

    for (int i = 0; i < len_a; i++) {
        if (item_a->beta[i] != item_b->beta[i]) {
            return false;
        }
    }

    return true;
}

uint64_t hash_item_list(void* items_ptr){
    Item* items = *((Item**) items_ptr);
    uint64_t curr_hash_int = hash_item(&items[0]);
    for(int i=1;i<dynarray_length(items);i++){
        uint64_t tmp_hash = hash_item(&items[i]);
        curr_hash_int = hash_combine(curr_hash_int, tmp_hash);
    }

    return curr_hash_int;
}

bool hash_item_list_equal(void* a_ptr, void* b_ptr) {
    Item* list_a = *(Item**)a_ptr;
    Item* list_b = *(Item**)b_ptr;

    if (list_a == list_b) return true;
    if (list_a == NULL || list_b == NULL) return false;

    int len_a = dynarray_length(list_a);
    int len_b = dynarray_length(list_b);
    if (len_a != len_b) return false;

    for (int i = 0; i < len_a; i++) {
        if (!hash_item_equal(&list_a[i], &list_b[i])) {
            return false;
        }
    }

    return true;
}

void print_item(Item item){
    printf("[ %d -> ", item.alpha);
    for(int i = 0;i<dynarray_length(item.beta);i++){
        if(i != -1 && i == item.k){
            printf("*");
        }
        printf("%d ", item.beta[i]);
    }
    printf(", %d ]\n", item.lookahead);
}

void print_production(Production prod){
    printf("[ %d -> ", prod.alpha);
    for(int i = 0;i<dynarray_length(prod.beta);i++){
        printf("%d ", prod.beta[i]);
    }
    printf("]\n", prod.alpha);
}

void print_grammar(Grammar G){
    printf("Goal: %d\n", G.S);
    printf("Terminals\n [");
    for(int i = 0;i<dynarray_length(G.T);i++){
        printf("%d, ", G.T[i]);
    }
    printf("]\n");
    printf("Non Terminals\n [");
    for(int i = 0;i<dynarray_length(G.NT);i++){
        printf("%d, ", G.NT[i]);
    }
    printf("]\n");
    printf("Production Rules\n");
    for(int i = 0;i<dynarray_length(G.productions);i++){
        printf("%d | ", i);
        print_production(G.productions[i]);
    }
}

Grammar create_grammar(){
    Grammar G;
    G.T = dynarray_create(int);
    G.NT = dynarray_create(int);
    G.productions = dynarray_create(Production);
    G.S = 0;
    return G;
}

Production create_production(int a, int* b, int b_count){
    Production production;
    production.alpha = a;
    production.beta = dynarray_create(int);
    for(int i = 0;i<b_count;i++){
        int bi = b[i];
        dynarray_push(production.beta, bi);
    }

    return production;
}

Subset* generate_first(Grammar G){
    int symbols_length = dynarray_length(G.T)+dynarray_length(G.NT);
    Subset* first = malloc(symbols_length*sizeof(Subset));
    for(int i = 0;i<dynarray_length(G.T);i++){
        int symbol = G.T[i];
        first[symbol] = SS_initialize_empty(symbols_length);
        SS_add(&first[symbol], symbol);
    }
    for(int i = 0;i<dynarray_length(G.NT);i++){
        int symbol = G.NT[i];
        first[symbol] = SS_initialize_empty(symbols_length);
    }

    bool changed = false;
    do{
        changed = false;
        for(int i = 0;i<dynarray_length(G.productions);i++){
            int* B = G.productions[i].beta;
            int A = G.productions[i].alpha;
            int prod_beta_size = dynarray_length(B);
            bool contains_empty = false;

            Subset rhs = SS_deep_copy(first[B[0]]);
            SS_remove(&rhs, EPSILON);

            int k = 0;
            for(int j = 0;j<prod_beta_size;j++){
                if(B[j] == EPSILON){
                    contains_empty = true;
                    break;
                }
            }

            if(!contains_empty){
                while(SS_in(first[B[k]], EPSILON) && i<prod_beta_size-1){
                    Subset temp_rhs = SS_deep_copy(first[B[k+1]]);
                    SS_remove(&temp_rhs, EPSILON);
                    SS_union(rhs, temp_rhs);
                    SS_destroy(&temp_rhs);
                    k++;
                }   
            }
            if(k==prod_beta_size-1 && SS_in(first[B[k]], EPSILON)){
                SS_add(&rhs, EPSILON);
            }
            Subset first_A_tmp = SS_deep_copy(first[A]);
            SS_union(first[A], rhs);
            if(!SS_equal(first[A], first_A_tmp)){
                changed = true;
            }

            SS_destroy(&first_A_tmp);
            SS_destroy(&rhs);
            
        }
    }
    while(changed == true);
    return first;
}

Item* closure(Grammar G, Item* s_raw, Subset* first){
    bool change = false;
    Item* s = dynarray_copy(s_raw);
    int counter = 0;
    do{
        //printf("SHIT ---\n");
        //for(int PUTA = 0;PUTA<dynarray_length(s);PUTA++){
            //print_item(s[PUTA]);
        //}
        //printf("SHIT ---\n");
        change = false;
        for(int i = 0;i<dynarray_length(s);i++){
            int curr_k = s[i].k;
            int C = s[i].beta[curr_k];
            int beta_length = dynarray_length(s[i].beta);
            for(int j = 0;j<dynarray_length(G.productions);j++){
                if(G.productions[j].alpha == C){

                    bool found_delta_first = false;
                    for(int delta_index=curr_k+1;delta_index<beta_length;delta_index++){
                        int delta = s[i].beta[delta_index];
                        if(delta == EPSILON){
                            continue;
                        }
                        found_delta_first = true;

                        Subset delta_first = first[delta];
                        int* delta_first_list = SS_to_list_indexes(delta_first);
                        for(int b=0;b<dynarray_length(delta_first_list);b++){
                            Item new_item;
                            new_item.alpha = C;
                            new_item.beta = G.productions[j].beta;
                            new_item.lookahead = delta_first_list[b];
                            new_item.k = 0;
                            
                            if(!item_in(s, new_item)){
                                dynarray_push(s, new_item);
                                change = true;
                            }
                            
                        } 
                    }    

                    if(found_delta_first == false){
                        Subset delta_first = first[s[i].lookahead];
                        int* delta_first_list = SS_to_list_indexes(delta_first);
                        for(int b=0;b<dynarray_length(delta_first_list);b++){
                            Item new_item;
                            new_item.alpha = C;
                            new_item.beta = G.productions[j].beta;
                            new_item.lookahead = delta_first_list[b];
                            new_item.k = 0;
                            
                            if(!item_in(s, new_item)){
                                dynarray_push(s, new_item);
                                change = true;
                            }
                        }
                    }
                }
            }
        }
        counter++;
    } while(change == true);

    return s;
}

Item* goto_table(Grammar G, Item* s_raw, Subset* first, int x){
    Item* s = dynarray_copy(s_raw);
    Item* moved = dynarray_create(Item);
    
    for(int i = 0;i<dynarray_length(s);i++){
        int k_pos = s[i].k;
        if(k_pos < dynarray_length(s[i].beta) && s[i].beta[k_pos] == x){
            Item new_item = item_copy(s[i]);
            new_item.k ++;

            dynarray_push(moved, new_item);
        }
    }

    //dynarray_destroy(s);

    return closure(G, moved, first);
}

void c_collection(Grammar G, Subset* first){
    Item start_item;
    start_item.alpha = G.productions[0].alpha;
    start_item.beta = G.productions[1].beta;
    start_item.lookahead = END;
    start_item.k = 0;

    Item* s = dynarray_create(Item);
    dynarray_push(s, start_item);

    CC_Item cc0;
    cc0.cc = closure(G, s, first);
    cc0.marked = false;

    CC_Item* CC = dynarray_create(CC_Item);
    Hash HCC = hash_create(2048, Item*, hash_item_list);
    dynarray_push(CC, cc0);
    hash_add(HCC, cc0.cc, hash_item_list_equal);

    bool added_set = true;
    while(added_set){
        added_set = false;
        for(int i=0;i<dynarray_length(CC);i++){
            if(CC[i].marked == false){
                Subset char_trans = SS_initialize_empty(dynarray_length(G.T)+dynarray_length(G.NT));
                Item* current_cc = CC[i].cc;
                for(int j=0;j<dynarray_length(current_cc);j++){
                    int curr_k = current_cc[j].k;
                    int* curr_beta = current_cc[j].beta;
                    if(curr_k<dynarray_length(curr_beta)){
                        SS_add(&char_trans, curr_beta[curr_k]);
                    }
                }

                for(int j=0;j<char_trans.capacity;j++){
                    if(char_trans.table[j]==true){
                        Item* temp = goto_table(G, current_cc, first, j);
                        if(hash_add(HCC, temp, hash_item_list_equal)){
                            dynarray_push(CC, temp);
                        }
                    }
                }

                SS_print(char_trans);
            }
        }
    }
}

bool int_equal(void* a, void* b) {
    return *(int*)a == *(int*)b;
}

int main() {
    printf("Parser...\n");

    // Make a way for this initialization to be made only with rules and string not this bs
    Grammar G = create_grammar();
    int terminals[] = {LEFT_PAR, RIGHT_PAR, EPSILON, END};
    int non_terminals[] = {GOAL, LIST, PAIR};

    G.S = GOAL;

    int b0[] = {LIST};
    Production item0 = create_production(GOAL, b0, 1);
    dynarray_push(G.productions, item0);

    int b1[] = {LIST, PAIR};
    Production item1 = create_production(LIST, b1, 2);
    dynarray_push(G.productions, item1);

    int b2[] = {PAIR};
    Production item2 = create_production(LIST, b2, 1);
    dynarray_push(G.productions, item2);

    int b3[] = {LEFT_PAR, PAIR, RIGHT_PAR};
    Production item3 = create_production(PAIR, b3, 3);
    dynarray_push(G.productions, item3);

    int b4[] = {LEFT_PAR, RIGHT_PAR};
    Production item4 = create_production(PAIR, b4, 2);
    dynarray_push(G.productions, item4);

    for(int i = 0;i<4;i++){
        int t = terminals[i];
        dynarray_push(G.T, t);
    }
    for(int i = 0;i<3;i++){
        int nt = non_terminals[i];
        dynarray_push(G.NT, nt);
    }

    print_grammar(G);

    Subset* first = generate_first(G);
    for(int i = 0;i<7;i++){
        int* list = SS_to_list_indexes(first[i]);
        printf("%d\n", i);
        printf("---\n");
        for(int j = 0;j<dynarray_length(list);j++){
            printf("%d ", list[j]);
        }
        printf("\n\n");
    }
    
    Item initial_item;
    initial_item.alpha = GOAL;
    initial_item.beta = dynarray_create(int);
    dynarray_push_rval(initial_item.beta, LIST);
    initial_item.lookahead = END;
    initial_item.k = 0;

    Item* s = dynarray_create(Item);
    dynarray_push(s, initial_item);

    printf("What\n");
    printf("%d\n", dynarray_length(s));
    Item* c = closure(G, s, first);

    printf("--- closure ---\n");
    for(int i = 0;i<dynarray_length(c);i++){
        print_item(c[i]);
    }

    Item* g = goto_table(G, c, first, LEFT_PAR);

    printf("--- goto ---\n");
    for(int i = 0;i<dynarray_length(g);i++){
        print_item(g[i]);
    }

    c_collection(G, first);

    Hash my_map = hash_create(5, Item*, hash_item_list);

    //printf("%llu", hash_item_list(c));

    hash_add(my_map, g, hash_item_list_equal);
    hash_add(my_map, g, hash_item_list_equal);
    hash_add(my_map, c, hash_item_list_equal);
    hash_add(my_map, s, hash_item_list_equal);


    Item** tl = my_map.obj_storage;


    printf("AAAA");
    for(int i = 0;i<dynarray_length(tl);i++){
        printf("---\n");
        for(int j = 0;j<dynarray_length(tl[i]);j++){
            print_item(tl[i][j]);
        }
        printf("---\n");
    }
    
}