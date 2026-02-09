#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "dynarray.h"
#include "subset.h"
#include "hash.h"
#include "scanner.h"
#include "re_pp.h"
#include "tree.h"

#define DEFAULT_STACK_SIZE 3

enum {
    END,
    EPSILON_P,
    GOAL,
};

typedef struct Production{
    int alpha;
    int* beta;
} Production;

typedef struct Item{
    int alpha;
    int** beta;
    int lookahead;
    int k;
} Item;

typedef struct CC_Item{
    Item* cc;
    int state;
    bool marked;
} CC_Item;

typedef struct Grammar{
    int* T;
    int* NT;
    int S;
    Production* productions;
} Grammar;

// From Scanner
typedef struct LRTransition{
    int state_from;
    int state_to;
    int trans_symbol;
} LRTransition;

typedef union StackItem{
    Token token;
    void* s_ptr;
    int s_int;
} StackItem;

typedef struct TableMaterial{
    CC_Item* CC;
    LRTransition* goto_transitions;
} TableMaterial;

typedef struct TableMapping{
    int*** table_action;;
    int** table_goto;
    int states_count; 
    int t_count;
    int nt_count ;
    int* action_mapping;
    int* goto_mapping;
    int* symbols_mapping;
} TableMapping;

typedef struct Pair{
    char* key;
    int value;
} Pair;

void print_transition_single(LRTransition t, char** symbol_names) {
    printf("  State %d --( %s )--> State %d\n", 
           t.state_from, 
           symbol_names[t.trans_symbol], 
           t.state_to);
}

void export_transition_single(LRTransition t, char** symbol_names, FILE* out) {
    fprintf(out, "  State %d --( %s )--> State %d\n", 
           t.state_from, 
           symbol_names[t.trans_symbol], 
           t.state_to);
}

void export_transition_list(LRTransition* transitions, char** symbol_names, FILE* out) {
    int count = dynarray_length(transitions);
    
    fprintf(out, "\n=== LR(1) Transition Table (%d entries) ===\n", count);
    for (int i = 0; i < count; i++) {
        export_transition_single(transitions[i], symbol_names, out);
    }
    fprintf(out, "============================================\n");
}

void print_transition_list(LRTransition* transitions, char** symbol_names) {
    int count = dynarray_length(transitions);
    
    printf("\n=== LR(1) Transition Table (%d entries) ===\n", count);
    for (int i = 0; i < count; i++) {
        print_transition_single(transitions[i], symbol_names);
    }
    printf("============================================\n");
}

void print_transitions(LRTransition* transitions, int count, char** symbol_names, int num_terminals) {
    printf("\n%-12s | %-15s | %-12s | %-10s\n", "From State", "Symbol", "To State", "Type");
    printf("------------------------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        LRTransition t = transitions[i];
        
        const char* type = (t.trans_symbol < num_terminals) ? "SHIFT" : "GOTO";

        printf("State %-6d | %-15s | State %-7d | %-10s\n", 
               t.state_from, 
               symbol_names[t.trans_symbol], 
               t.state_to, 
               type);
    }
    printf("------------------------------------------------------------------\n");
}

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
    if(dynarray_length(*item1.beta) != dynarray_length(*item2.beta)){
        return false;
    }
    for(int i = 0;i<dynarray_length(*item1.beta);i++){
        if((*item1.beta)[i] != (*item2.beta)[i]){
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
    new_item.beta = original.beta;
    new_item.lookahead = original.lookahead;
    new_item.k = original.k;

    return new_item;
}

Item* item_list_copy(Item* original){
    Item* item_list = dynarray_create_prealloc(Item, dynarray_length(original));

    for(int i = 0;i<dynarray_length(original); i++){
        Item new_item = item_copy(original[i]);
        dynarray_push(item_list, new_item);
    }

    return item_list;
}

uint64_t hash_item(void* item_ptr){
    Item item = *((Item*) item_ptr);
    uint64_t curr_hash_int = hash_int(item.alpha);
    for(int i=0;i<dynarray_length(*item.beta);i++){
        uint64_t tmp_hash_beta = hash_int((*item.beta)[i]);
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

    int len_a = dynarray_length(*item_a->beta);
    int len_b = dynarray_length(*item_b->beta);

    if (len_a != len_b) return false;

    for (int i = 0; i < len_a; i++) {
        if ((*item_a->beta)[i] != (*item_b->beta)[i]) {
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

uint64_t hash_CC_item(void* CC_ptr){
    CC_Item* item = (CC_Item*) CC_ptr;
    return hash_item_list(&item->cc);
}

bool hash_CC_item_equal(void* a_ptr, void* b_ptr){
    CC_Item* item_a = (CC_Item*)a_ptr;
    CC_Item* item_b = (CC_Item*)b_ptr;

    return(hash_item_list_equal(&item_a->cc, &item_b->cc));
}


int get_rhs_width(Item item, char** index_mapping) {
    int width = 0;
    int len = dynarray_length(*item.beta);
    for (int i = 0; i < len; i++) {
        width += strlen(index_mapping[(*item.beta)[i]]) + 1;
    }

    return width + 1; 
}

void export_item(Item item, char** index_mapping, int max_alpha, int max_rhs, FILE* out) {
    fprintf(out, "[ %-*s -> ", max_alpha, index_mapping[item.alpha]);

    int current_rhs_width = 0;
    int len = dynarray_length(*item.beta);
    for (int i = 0; i < len; i++) {
        if (i == item.k) {
            current_rhs_width += fprintf(out, "*");
        }
        current_rhs_width += fprintf(out, "%s ", index_mapping[(*item.beta)[i]]);
    }
    if (item.k == len) {
        current_rhs_width += fprintf(out, "*");
    }

    int padding = max_rhs - current_rhs_width;
    if (padding > 0) fprintf(out, "%*s", padding, "");

    fprintf(out, ", %s ]\n", index_mapping[item.lookahead]);
}

void print_item(Item item, char** index_mapping, int max_alpha, int max_rhs) {
    export_item(item , index_mapping, max_alpha, max_rhs, stdout);
}

void export_item_list(Item* c, char** val_table, char* title, FILE* out) {
    int max_alpha = 0;
    int max_rhs = 0;
    int max_lookahead = 0;
    int count = dynarray_length(c);

    for (int i = 0; i < count; i++) {
        int alpha_len = strlen(val_table[c[i].alpha]);
        if (alpha_len > max_alpha) max_alpha = alpha_len;

        int rhs_len = get_rhs_width(c[i], val_table);
        if (rhs_len > max_rhs) max_rhs = rhs_len;

        int la_len = strlen(val_table[c[i].lookahead]);
        if (la_len > max_lookahead) max_lookahead = la_len;
    }

    int total_width = 2 + max_alpha + 4 + max_rhs + 2 + max_lookahead + 2;

    int title_len = strlen(title);
    int dash_count = (total_width - title_len - 2) / 2;

    for(int i = 0; i < dash_count; i++) fprintf(out, "-");
    fprintf(out, " %s ", title);
    
    int remaining_dashes = total_width - title_len - 2 - dash_count;
    for(int i = 0; i < remaining_dashes; i++) fprintf(out, "-");
    fprintf(out, "\n");

    for (int i = 0; i < count; i++) {
        export_item(c[i], val_table, max_alpha, max_rhs, out);
    }
    
    for(int i = 0; i < total_width; i++) fprintf(out, "-");
    fprintf(out, "\n");
}

void print_item_list(Item* c, char** val_table, char* title) {
    export_item_list(c, val_table, title, stdout);
}

void export_production(Production prod, char** index_mapping, FILE* out){
    fprintf(out, "[ %s -> ", index_mapping[prod.alpha]);
    for(int i = 0;i<dynarray_length(prod.beta);i++){
        fprintf(out, "%s ", index_mapping[prod.beta[i]]);
    }
    fprintf(out, "]\n", prod.alpha);
}

void print_production(Production prod, char** index_mapping){
    printf("[ %s -> ", index_mapping[prod.alpha]);
    for(int i = 0;i<dynarray_length(prod.beta);i++){
        printf("%s ", index_mapping[prod.beta[i]]);
    }
    printf("]\n", prod.alpha);
}

void export_grammar(Grammar G, char** index_mapping, FILE* out){
    fprintf(out, "Goal: %s\n", index_mapping[G.S]);
    fprintf(out, "Terminals\n [");
    for(int i = 0;i<dynarray_length(G.T);i++){
        fprintf(out, "%s, ", index_mapping[G.T[i]]);
    }
    fprintf(out, "]\n");
    fprintf(out, "Non Terminals\n [");
    for(int i = 0;i<dynarray_length(G.NT);i++){
        fprintf(out, "%s, ", index_mapping[G.NT[i]]);
    }
    fprintf(out, "]\n");
    fprintf(out, "Production Rules\n");
    for(int i = 0;i<dynarray_length(G.productions);i++){
        fprintf(out, "%d | ", i);
        export_production(G.productions[i], index_mapping, out);
    }
}

void print_grammar(Grammar G, char** index_mapping){
    export_grammar(G, index_mapping, stdout);
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

Production destroy_production(Production* production){
    dynarray_destroy(production->beta);
}

void destroy_grammar(Grammar* G){
    dynarray_destroy(G->T);
    dynarray_destroy(G->NT);
    for(int i = 0;i<dynarray_length(G->productions);i++){
        destroy_production(&(G->productions[i]));
    }
    dynarray_destroy(G->productions);
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
            SS_remove(&rhs, EPSILON_P);

            int k = 0;
            for(int j = 0;j<prod_beta_size;j++){
                if(B[j] == EPSILON_P){
                    contains_empty = true;
                    break;
                }
            }

            if(!contains_empty){
                while(SS_in(first[B[k]], EPSILON_P) && i<prod_beta_size-1){
                    Subset temp_rhs = SS_deep_copy(first[B[k+1]]);
                    SS_remove(&temp_rhs, EPSILON_P);
                    SS_union(rhs, temp_rhs);
                    SS_destroy(&temp_rhs);
                    k++;
                }   
            }
            if(k==prod_beta_size-1 && SS_in(first[B[k]], EPSILON_P)){
                SS_add(&rhs, EPSILON_P);
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

void destroy_first(Grammar G, Subset* first){
    int symbols_length = dynarray_length(G.T)+dynarray_length(G.NT);
    for(int i = 0;i<symbols_length;i++){
        SS_destroy(&first[i]);
    }
    free(first);
}


void export_first_sets(Grammar G, Subset* first, char** val_table, FILE* out) {
    fprintf(out, "\n--- FIRST SETS ---\n");
    
    for (int i = 0; i < dynarray_length(G.NT); i++) {
        int nt_id = G.NT[i];
        fprintf(out, "FIRST(%s) = { ", val_table[nt_id]);

        bool first_element = true;

        int total_symbols = dynarray_length(G.T) + dynarray_length(G.NT);
        
        for (int s = 0; s < total_symbols; s++) {
            if (SS_in(first[nt_id], s)) {
                if (!first_element) fprintf(out, ", ");
                
                if (s == EPSILON_P) fprintf(out, "ε");
                else fprintf(out, "%s", val_table[s]);
                
                first_element = false;
            }
        }
        fprintf(out, " }\n");
    }
    fprintf(out, "------------------\n");
}

void print_first_sets(Grammar G, Subset* first, char** val_table) {
    export_first_sets(G, first, val_table, stdout);
}


Item* item_closure(Grammar G, Item* s_raw, Subset* first){
    bool change = false;
    Item* s = item_list_copy(s_raw);
    int counter = 0;

    //printf("SHIT ---\n");
        //for(int PUTA = 0;PUTA<dynarray_length(s);PUTA++){
            //print_item(s[PUTA]);
        //}
    //printf("SHIT ---\n");
    do{
        change = false;
        for(int i = 0;i<dynarray_length(s);i++){
            int curr_k = s[i].k;
            int C = (*s[i].beta)[curr_k];
            int beta_length = dynarray_length(*s[i].beta);
            for(int j = 0;j<dynarray_length(G.productions);j++){
                if(G.productions[j].alpha == C){

                    bool found_delta_first = false;
                    for(int delta_index=curr_k+1;delta_index<beta_length;delta_index++){
                        int delta = (*s[i].beta)[delta_index];
                        if(delta == EPSILON_P){
                            continue;
                        }

                        Subset delta_first = first[delta];
                        int* delta_first_list = SS_to_list_indexes(delta_first);
                        for(int b=0;b<dynarray_length(delta_first_list);b++){
                            Item new_item;
                            new_item.alpha = C;
                            new_item.beta = &G.productions[j].beta;
                            new_item.lookahead = delta_first_list[b];
                            new_item.k = 0;
                            
                            if(!item_in(s, new_item)){
                                dynarray_push(s, new_item);
                                change = true;
                            }
                            
                        } 

                        found_delta_first = true;
                        dynarray_destroy(delta_first_list);
                        break;
                    }

                    if(found_delta_first == false){
                        Subset delta_first = first[s[i].lookahead];
                        int* delta_first_list = SS_to_list_indexes(delta_first);
                        for(int b=0;b<dynarray_length(delta_first_list);b++){
                            Item new_item;
                            new_item.alpha = C;
                            new_item.beta = &G.productions[j].beta;
                            new_item.lookahead = delta_first_list[b];
                            new_item.k = 0;

                            //printf("KURWA ---\n");
                            //for(int PUTA = 0;PUTA<dynarray_length(s);PUTA++){
                                //print_item(s[PUTA]);
                            //}
                            //printf("KURWA ---\n");
                            
                            if(!item_in(s, new_item)){
                                dynarray_push(s, new_item);
                                change = true;
                            }
                        }

                        dynarray_destroy(delta_first_list);
                    }
                }
            }
        }
        counter++;
    } while(change == true);

    return s;
}

Item* goto_table(Grammar G, Item* s, Subset* first, int x){
    Item* moved = dynarray_create(Item);
    
    for(int i = 0;i<dynarray_length(s);i++){
        int k_pos = s[i].k;
        if(k_pos < dynarray_length(*s[i].beta) && (*s[i].beta)[k_pos] == x){
            Item new_item = item_copy(s[i]);
            new_item.k ++;

            dynarray_push(moved, new_item);
        }
    }

    Item* closure = item_closure(G, moved, first);
    // FUCK
    dynarray_destroy(moved);

    return closure;
}

TableMaterial c_collection(Grammar G, Subset* first){
    Item start_item;
    start_item.alpha = G.productions[0].alpha;
    start_item.beta = &G.productions[0].beta;
    start_item.lookahead = END;
    start_item.k = 0;

    Item* s = dynarray_create(Item);
    dynarray_push(s, start_item);

    CC_Item cc0;
    cc0.cc = item_closure(G, s, first);
    
    dynarray_destroy(s);

    cc0.marked = false;
    cc0.state = 0;

    CC_Item* CC = dynarray_create(CC_Item);
    LRTransition* trans = dynarray_create(LRTransition);
    Hash HCC = hash_create(2048, CC_Item, hash_CC_item);
    dynarray_push(CC, cc0);
    hash_add(HCC, cc0, hash_CC_item_equal);

    bool added_set = true;
    while(added_set){
        added_set = false;
        for(int i=0;i<dynarray_length(CC);i++){
            if(CC[i].marked == false){
                Subset char_trans = SS_initialize_empty(dynarray_length(G.T)+dynarray_length(G.NT));
                Item* current_cc = CC[i].cc;
                CC[i].marked = true;
                for(int j=0;j<dynarray_length(current_cc);j++){
                    int curr_k = current_cc[j].k;
                    int* curr_beta = *current_cc[j].beta;
                    if(curr_k<dynarray_length(curr_beta)){
                        SS_add(&char_trans, curr_beta[curr_k]);
                    }
                }

                for(int j=0;j<char_trans.capacity;j++){
                    if(char_trans.table[j]==true){
                        Item* temp = goto_table(G, current_cc, first, j);
                        CC_Item temp_item;
                        LRTransition new_transition;
                        new_transition.state_from = i;
                        new_transition.trans_symbol = j;

                        temp_item.cc = temp;
                        temp_item.marked = false;
                        temp_item.state = dynarray_length(CC);
                        if(!hash_add(HCC, temp_item, hash_CC_item_equal)){
                            new_transition.state_to = temp_item.state;
                            added_set = true;
                            dynarray_push(CC, temp_item);
                        }
                        else{
                            void* stored_ptr = hash_get(HCC, temp_item, hash_CC_item_equal);
                            if (stored_ptr != NULL) {
                                CC_Item* stored_item = (CC_Item*) stored_ptr;
                                new_transition.state_to = stored_item->state;
                                //printf("Lengths: %d\n", dynarray_length(CC));
                                //printf("Stored State: %d\n", stored_item->state);
                            }

                            dynarray_destroy(temp);
                        }

                        dynarray_push(trans, new_transition);
                    }
                }

                SS_destroy(&char_trans);
            }
        }
    }

    //printf("OK OK OK?\n");
    //for(int i = 0;i<dynarray_length(CC);i++){
        //printf("---\n");
        //for(int j = 0;j<dynarray_length(CC[i].cc);j++){
            //print_item(CC[i].cc[j]);
        //}
        //printf("---\n");
    //}

    //for(int i = 0;i<dynarray_length(trans);i++){
        //printf("---\n");
        //print_transition(trans[i]);
    //}

    hash_destroy(HCC);

    TableMaterial fout;
    fout.CC = CC;
    fout.goto_transitions = trans;
    return fout;
}

void export_canonical_collection(CC_Item* CC, char** val_table, FILE* out) {
    int count = dynarray_length(CC);
    
    fprintf(out, "\n=== CANONICAL COLLECTION ===\n");
    
    for (int i = 0; i < count; i++) {
        char title_buffer[32];
        sprintf(title_buffer, "State %d", CC[i].state);
        
        export_item_list(CC[i].cc, val_table, title_buffer, out);
        
        fprintf(out, "\n");
    }
    
    fprintf(out, "Total States: %d\n", count);
    fprintf(out, "============================\n");
}

void print_canonical_collection(CC_Item* CC, char** val_table) {
    export_canonical_collection(CC, val_table, stdout);
}

void export_tables(TableMapping* tm, FILE* out) {
    if (tm == NULL || out == NULL) return;

    fprintf(out, "\n--- LR(1) PARSER TABLES (Canonical Closure) ---\n\n");

    fprintf(out, "%-5s |", "State");
    for (int j = 0; j < tm->t_count; j++) {
        fprintf(out, " T%-3d |", tm->action_mapping[j]);
    }
    for (int j = 0; j < tm->nt_count; j++) {
        fprintf(out, " NT%-2d |", tm->goto_mapping[j]);
    }
    fprintf(out, "\n");

    int total_width = 7 + ((tm->t_count + tm->nt_count) * 8);
    for (int i = 0; i < total_width; i++) fputc('-', out);
    fprintf(out, "\n");

    for (int i = 0; i < tm->states_count; i++) {
        fprintf(out, "%-5d |", i);

        for (int j = 0; j < tm->t_count; j++) {
            int action_type = tm->table_action[i][j][0]; 
            int action_val  = tm->table_action[i][j][1];

            if (action_type == 2) {
                fprintf(out, " s%-3d |", action_val);
            } else if (action_type == 1) {
                fprintf(out, " acc  |");
            } else if (action_val == 3) {
                fprintf(out, " r%-3d |", action_type + 1); 
            } else {
                fprintf(out, "      |");
            }
        }

        for (int j = 0; j < tm->nt_count; j++) {
            int state_to = tm->table_goto[i][j];
            if (state_to != -1) {
                fprintf(out, " %-4d |", state_to);
            } else {
                fprintf(out, "      |");
            }
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n");
}

void print_tables(TableMapping* tm) {
    export_tables(tm, stdout);
}

TableMapping create_tables(Grammar G, TableMaterial tb){
    int t_length = dynarray_length(G.T);
    int nt_length = dynarray_length(G.NT);
    int states_count = dynarray_length(tb.CC);

    Subset fast_terminal = SS_initialize(t_length+nt_length, G.T,t_length);
    Subset counted = SS_initialize_empty(t_length+nt_length);
    int* symbols_mapping = malloc((t_length+nt_length) * sizeof(int));
    int* action_mapping = dynarray_create(int);
    int* goto_mapping = dynarray_create(int);
    

    int t_count = 1;
    int nt_count = 0;

    SS_add(&counted, END);
    symbols_mapping[END] = 0;

    int end_rval = END;
    dynarray_push(action_mapping, end_rval);

    //printf("--- Mappings ---\n");

    for(int i=0;i<dynarray_length(tb.goto_transitions);i++){
        int curr_symbol = tb.goto_transitions[i].trans_symbol;
        if(!SS_in(counted, curr_symbol)){
            if(SS_in(fast_terminal,curr_symbol)){
                symbols_mapping[curr_symbol] = t_count;
                dynarray_push(action_mapping, curr_symbol);
                //printf("Terminal[%d] = %d\n", curr_symbol, t_count);
                t_count ++;
            }
            else{
                symbols_mapping[curr_symbol] = nt_count;
                dynarray_push(goto_mapping, curr_symbol);
                //printf("Non-Terminal[%d] = %d\n", curr_symbol, nt_count);
                nt_count ++;
            }
            SS_add(&counted, curr_symbol);
        }
    }

    SS_destroy(&counted);

    //printf("--- Counts ---\n");
    //printf("T: %d, NT: %d\n", t_count, nt_count);
    //printf("--- Actions ---\n");

    int*** table_action = malloc(states_count * sizeof(int**));
    for(int i=0;i<dynarray_length(tb.CC);i++){
        table_action[i] = calloc(t_count, sizeof(int*));
        for(int j=0;j<t_count;j++){
            table_action[i][j] = calloc(2, sizeof(int));
        }
    }

    int** table_goto = malloc(states_count * sizeof(int*));
    for(int i=0;i<dynarray_length(tb.CC);i++){
        table_goto[i] = malloc(nt_count * sizeof(int));
        memset(table_goto[i], -1, nt_count * sizeof(int));
    }
    
    for(int i=0;i<dynarray_length(tb.goto_transitions);i++){
        LRTransition curr_trans = tb.goto_transitions[i];
        if(SS_in(fast_terminal, curr_trans.trans_symbol)){
            //printf("Action[i->%d, c->%d] = shift j->%d\n", curr_trans.state_from, curr_trans.trans_symbol, curr_trans.state_to);
            if(table_action[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]][0] == 3){
                printf("Shift Reduce Conflict at state: %d symbol: %d\n", i, curr_trans.trans_symbol);
            }

            table_action[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]][0] = 2;
            table_action[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]][1] = curr_trans.state_to;
        }
        else{
            //printf("Goto[i->%d, n->%d] = j->%d\n", curr_trans.state_from, curr_trans.trans_symbol, curr_trans.state_to);
            table_goto[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]] = curr_trans.state_to;
        }
    }

    SS_destroy(&fast_terminal);

    for(int i=0;i<dynarray_length(tb.CC);i++){
        for(int j=0;j<dynarray_length(tb.CC[i].cc);j++){
            Item curr_item = tb.CC[i].cc[j];
            int* curr_beta = *curr_item.beta;
            if(curr_item.k == dynarray_length(curr_beta)){
                if(curr_item.alpha == GOAL){
                    //printf("Action[i->%d, a->%d] = acc\n", i, curr_item.lookahead);
                    table_action[i][symbols_mapping[curr_item.lookahead]][0] = 1;
                }
                else{
                    int p_rule = -1;
                    for(int k=0;k<dynarray_length(G.productions);k++){
                        if(G.productions[k].alpha == curr_item.alpha && G.productions[k].beta == *curr_item.beta){
                            p_rule = k;
                            break;
                        }
                    }

                    assert(table_action[i][symbols_mapping[curr_item.lookahead]][0] != 3);

                    //printf("Action[i->%d, a->%d] = reduce p->%d\n", i, curr_item.lookahead, p_rule+1);
                    if(table_action[i][symbols_mapping[curr_item.lookahead]][0] == 2){
                        printf("Shift Reduce Conflict at state: %d symbol: %d\n", i, curr_item.lookahead);
                    }
                    else if(table_action[i][symbols_mapping[curr_item.lookahead]][0] == 3){
                        printf("Reduce Reduce Conflict at state: %d symbol: %d\n", i, curr_item.lookahead);
                    }
                    else{
                        table_action[i][symbols_mapping[curr_item.lookahead]][0] = 3;
                        table_action[i][symbols_mapping[curr_item.lookahead]][1] = p_rule;
                    }
                }
            }
        }
    }

    dynarray_destroy(tb.goto_transitions);
    for(int i = 0;i<dynarray_length(tb.CC);i++){
        dynarray_destroy(tb.CC[i].cc);
    }
    dynarray_destroy(tb.CC);
    
    TableMapping t_mapping;
    t_mapping.table_action = table_action;
    t_mapping.table_goto = table_goto;
    t_mapping.states_count = states_count;
    t_mapping.t_count = t_count;
    t_mapping.nt_count = nt_count;
    t_mapping.action_mapping = action_mapping;
    t_mapping.goto_mapping = goto_mapping;
    t_mapping.symbols_mapping = symbols_mapping;

    return t_mapping;
}


void destroy_tables(TableMapping t_mapping){
    for(int i=0;i<t_mapping.states_count;i++){
        for(int j=0;j<t_mapping.t_count;j++){
            free(t_mapping.table_action[i][j]);
        }
        free(t_mapping.table_action[i]);
    }

    free(t_mapping.table_action);

    for(int i=0;i<t_mapping.states_count;i++){
        free(t_mapping.table_goto[i]);
    }

    free(t_mapping.table_goto);

    free(t_mapping.symbols_mapping);

    dynarray_destroy(t_mapping.action_mapping);
    dynarray_destroy(t_mapping.goto_mapping);
}

void print_stack(StackItem* stack, char** index_mapping) {
    int len = dynarray_length(stack);
    printf("--- Stack ---\n");
    printf("[ ");
    
    for (int i = 0; i < len; i++) {
        if (i % DEFAULT_STACK_SIZE == 1) {
            printf("%s ", index_mapping[stack[i].token.category]);
        }
        else if (i % DEFAULT_STACK_SIZE == 2){
            printf("%d ", stack[i].s_int);
        }
        else if (i % DEFAULT_STACK_SIZE == 0){
            printf("%d ", ((TreeNode*) (stack[i].s_ptr))->children_amount);
        }
    }
    
    printf("]\n");
}

Hash dictionary_from_mapping(Pair* mapping, int map_size){
    Hash dict_map = dynadict_create(512, int);
    for(int i=0;i<map_size;i++){
        //printf("DICT MAPPING: %d %s %d\n", i, mapping[i].key, mapping[i].value);
        char* key = mapping[i].key;
        int value = mapping[i].value;
        dynadict_add(dict_map, key, value);
    }

    return dict_map;
}

char** storage_table_from_mapping(Pair* mapping, int map_size){
    char** inverse_map = malloc(map_size * sizeof(char*));
    for(int i=0;i<map_size;i++){
        inverse_map[i] = mapping[i].key;
    }

    return inverse_map;
}


TreeNode* parser_skeleton(Grammar G, TableMapping tb, Token* token_ptr, int extra_parameters, char** index_mapping){

    StackItem* stack = dynarray_create(StackItem);
    //StackItem* token_bs = malloc((2+extra_parameters)*2*sizeof(StackItem));

    StackItem first_node;
    StackItem first_word;
    StackItem first_state;

    first_state.s_int = 0;

    first_node.s_ptr = tree_make_node(0, "Root", NULL);
    first_word.token.word = "";
    first_word.token.category = END;

    dynarray_push(stack, first_node);
    dynarray_push(stack, first_word);
    dynarray_push(stack, first_state);

    //printf("STACK: %d\n", first_state.s_int);

    do{
        StackItem top_state = dynarray_get_last(stack);

        printf("--- Iteration ---\n");
        printf("Current Word: %s\n", index_mapping[token_ptr->category]);
        printf("Current State: %d\n", top_state.s_int);
        print_stack(stack, index_mapping);
        //printf("Stack Top-> %d\n", top_state.s_int);
        //printf("Current Word-> %s\n", token_ptr->word);

        int word_category_table = tb.symbols_mapping[token_ptr->category];

        if(tb.table_action[top_state.s_int][word_category_table][0] == 3){

            int prod_rule = tb.table_action[top_state.s_int][word_category_table][1];
            int A = G.productions[prod_rule].alpha;
            int* beta = G.productions[prod_rule].beta;

            StackItem new_token;
            new_token.token.word = index_mapping[A];
            new_token.token.category = A;
            
            TreeNode** children = malloc(dynarray_length(beta)*sizeof(TreeNode*));
            for(int i=0;i<dynarray_length(beta);i++){
                int s = dynarray_length(stack)-((i+1)*(DEFAULT_STACK_SIZE+extra_parameters));
                assert(s>0);
                children[i] = (TreeNode*) stack[s].s_ptr;
                //printf("MAYBE\n");
                //printf("%d, %d\n", s, dynarray_length(stack));
                //print_node_info(children[i]);
            }

            TreeNode* tmp_node = tree_make_node(dynarray_length(beta), new_token.token.word, children);

            //printf("TMP NODE");
            //print_node_info(tmp_node);

            free(children);

            for(int i=0;i<(DEFAULT_STACK_SIZE+extra_parameters)*dynarray_length(beta);i++){
                StackItem trash;
                dynarray_pop(stack, &trash);
            }
            
            //printf("stack get %d\n", dynarray_get_last(stack).s_int);
            //printf("already_mapped %d\n", tb.symbols_mapping[A]);
            int to_state = tb.table_goto[dynarray_get_last(stack).s_int][tb.symbols_mapping[A]];
            
            StackItem new_node;
            StackItem new_state;
            
            new_node.s_ptr = tmp_node;
            new_state.s_int = to_state;
            
            dynarray_push(stack, new_node);
            dynarray_push(stack, new_token);
            dynarray_push(stack, new_state);
           
            
            printf("Reduce -> %d\n", prod_rule+1); 
        }
        else if(tb.table_action[top_state.s_int][word_category_table][0] == 2){
            int to_state = tb.table_action[top_state.s_int][word_category_table][1];
            
            StackItem new_node;
            StackItem new_token;
            StackItem new_state;

            TreeNode* tmp_node = tree_make_node(0, token_ptr->word, NULL);
            
            //printf("TMP NODE");
            //print_node_info(tmp_node);
            
            new_node.s_ptr = tmp_node;
            new_token.token.word = token_ptr->word;
            new_token.token.category = token_ptr->category;

            new_state.s_int = to_state;

            dynarray_push(stack, new_node);
            dynarray_push(stack, new_token);
            dynarray_push(stack, new_state);

            //printf("STACK: %d\n", new_state.s_int);

            token_ptr++;

            printf("Shift -> %d\n", to_state);
        }
        else if(tb.table_action[top_state.s_int][word_category_table][0] == 1){
            printf("Accept\n");
            break;
        }
        else{
            printf("Error\n");
            break;
        }

        //printf("%s", token_ptr->word);
    } while(true);

    TreeNode* root = (TreeNode*) stack[DEFAULT_STACK_SIZE+extra_parameters].s_ptr;

    dynarray_destroy(stack);
    tree_destroy_node(first_node.s_ptr);

    return root;
}

bool int_equal(void* a, void* b) {
    return *(int*)a == *(int*)b;
}

Grammar build_grammar(FA rules_regex, char *file_lexing_rules, Hash dict_mapping, int symbols_amount, FILE* out){
    int ignore_categories[] = {1};
    Token* token_anchor = scanner_loop_file(rules_regex, file_lexing_rules, ignore_categories, 1);
    Token* token = token_anchor;

    export_token_seq(token, out);

    Grammar G = create_grammar();
    Subset non_terminals_ss = SS_initialize_empty(symbols_amount);

    int state = 0;
    int head_word_class;
    int* beta_memory = dynarray_create(int);
    bool first_creation = true;
    while(token->category != 0){
        if(state == 0 && token->category == 2){
            int* pointer_get = dynadict_get(dict_mapping,token->word);
            printf("word: %s \n", token->word);
            if(pointer_get == NULL){
                printf("Rules Synthax Error\n");
            }
            assert(pointer_get != NULL);
            head_word_class = *pointer_get;
            SS_add(&non_terminals_ss, head_word_class);
            if(first_creation){
                G.S = head_word_class;
            }
            first_creation = false;
            state = 1;
        }
        else if(state == 0 && token->category == 3){
            state = 1;
        }
        else if(state == 1 && token->category == 4){
            state = 2;
        }
        else if(state == 2 && token->category == 2){
            int* pointer_get = dynadict_get(dict_mapping,token->word);
            //printf("WHAT TF IS THIS %s\n", token->word);
            //printf("WHAT TF IS THIS %d\n", *pointer_get);
            if(pointer_get == NULL){
                printf("Rules Synthax Error\n");
                printf("Word Unrecognized: %s\n", token->word);
            }
            assert(pointer_get != NULL);
            int b_word_class = *pointer_get;
            dynarray_push(beta_memory, b_word_class);
            state = 2;
        }
        else if(state == 2 && token->category == 5){
            Production item = create_production(head_word_class, beta_memory, dynarray_length(beta_memory));
            dynarray_push(G.productions, item);
            beta_memory = dynarray_create(int);
            state = 0;
        }
        else{
            printf("Rules Synthax Error\n");
            assert(false);
        }

        token ++;
    }

    Subset terminals_ss = SS_deep_copy(non_terminals_ss);
    SS_inv(terminals_ss);
    
    G.NT = SS_to_list_indexes(non_terminals_ss);
    G.T = SS_to_list_indexes(terminals_ss);

    SS_destroy(&non_terminals_ss);
    SS_destroy(&terminals_ss);

    dynarray_destroy(token_anchor);

    return G;
}


int main(){
    printf("Parser...\n");

    Pair mapping[] = {
        {"End",             0},
        {"Epsilon",         1},
        {"Goal",            2},
        {"Expr",            3},
        {"Eval",            4},
        {"Term",            5},
        {"Factor",          6},
        {"+",               7},
        {"-",               8},
        {"*",               9},
        {"/",               10},
        {"(",               11},
        {")",               12},
        {"num",             13},
        {"name",            14},
        {"[",               15},
        {"]",               16},
        {".",               17},
        {",",               18},
        {"=?",              19},
        {">=",              20},
        {"<=",              21},
        {">",               22},
        {"<",               23},
        {"string",          24},
        {"true",            25},
        {"false",           26},
        {"Access",          27},
        {"AccessBase",      28},
        {"LoP",             29},
        {"Args",            30},
        {"ArgList",         31},
        {"if",              32}, 
        {"else",            33},
        {"while",           34},
        {"for",             35},
        {"Init",            36},
        {"Proc",            37},
        {"return",          38},
        {"{",               39},
        {"}",               40},
        {";",               41},
        {"<-",              42},
        {"=",               43},
        {":",               44},
        {"->",              45},
        {"int",             46},
        {"bool",            47},
        {"float",           48},
        {"break",           49},
        {"continue",        50},
        {"goto",            51},
        {"Program",         52},
        {"Block",           53},
        {"CompStat",        54},
        {"UnitStat",        55},
        {"ControlStat",     56},
        {"Stat",            57},
        {"CondStat",        58},
        {"LoopStat",        59},
        {"While",           60},
        {"For",             61},
        {"Declaration",     62},
        {"ProcDeclaration", 63},
        {"Assignment",      64},
        {"VarType",         65},
        {"Primitive",       66},
        {"Jump",            67},
        {"Params",          68},
        {"ParamsList",      69}
    };

    int symbols_amount = 70;
    Hash dict_map = dictionary_from_mapping(mapping, symbols_amount);
    char** value_map = storage_table_from_mapping(mapping, symbols_amount);

    // --- 2. GRAMMAR CONSTRUCTION ---
    char* prod_rules_src = "grammar.k.specs";
    char* re_rules = "(([a-zA-Z/(/)/*///-/[/]+=?><.;{},:])([a-zA-Z/(/)/*///-/[/]+=?><.;{},:])*)$02|///|$03|(//->)$04|//;$05|(( |\n|\t|\r)( |\n|\t|\r)*)$01";
    
    FA rules_regex = MakeFA(re_rules, "output/rules_dfa.txt", true);
    FILE* file_rules_seq = fopen("output/rules_seq.txt", "w");
    
    Grammar G = build_grammar(rules_regex, prod_rules_src, dict_map, symbols_amount, file_rules_seq);
    fclose(file_rules_seq);

    // Export Grammar
    print_grammar(G, value_map);
    FILE* file_grammar = fopen("output/grammar.txt", "w");
    export_grammar(G, value_map, file_grammar);
    fclose(file_grammar);

    FA_destroy(&rules_regex);

    // --- 3. FIRST SETS GENERATION ---
    Subset* first = generate_first(G);
    
    print_first_sets(G, first, value_map);
    FILE* file_first = fopen("output/first_sets.txt", "w");
    export_first_sets(G, first, value_map, file_first);
    fclose(file_first);

    // --- 4. CANONICAL COLLECTION & TRANSITIONS ---
    TableMaterial table_material = c_collection(G, first);

    destroy_first(G, first);

    print_canonical_collection(table_material.CC, value_map);
    print_transition_list(table_material.goto_transitions, value_map);

    FILE* file_collection = fopen("output/collection.txt", "w");
    export_canonical_collection(table_material.CC, value_map, file_collection);
    fprintf(file_collection, "\n\n\n");
    export_transition_list(table_material.goto_transitions, value_map, file_collection);
    fclose(file_collection);

    // --- 5. LR(1) TABLE MAPPING ---
    TableMapping tables_info = create_tables(G, table_material);
    
    
    print_tables(&tables_info);
    FILE* file_tables = fopen("output/parser_tables.txt", "w");
    export_tables(&tables_info, file_tables);
    fclose(file_tables);

    // --- 6. LEXER EXECUTION ---
    char* file_dir = "languaje.k";
    char* lexing_rules = "(=?)$19|(>=)$20|(<=)$21|(>)$22|(<)$23|+$07|-$08|/*$09|//$10|/($11|/)$12|/[$15|/]$16|.$17|,$18|(0|[1-9][0-9]*)$13|(\"([a-zA-Z0-9_][a-zA-Z0-9_]*)\")$24|(true)$25|(false)$26|(if)$32|(else)$33|(while)$34|(for)$35|(Init)$36|(Proc)$37|(return)$38|({)$39|(})$40|(;)$41|(<-)$42|(=)$43|(:)$44|(->)$45|(int)$46|(bool)$47|(float)$48|(break)$49|(continue)$50|(goto)$51|([a-zA-Z_][a-zA-Z0-9_]*)$14|(( |\n|\t|\r)( |\n|\t|\r)*)$01";
    int ignore_categories[] = {1};

    FA lexing_rules_regex = MakeFA(lexing_rules, "output/lexer_dfa.txt", true);
    Token* scanner_out = scanner_loop_file(lexing_rules_regex, file_dir, ignore_categories, 1);

    FA_destroy(&lexing_rules_regex);

    print_token_seq(scanner_out);
    FILE* file_lexer_seq = fopen("output/lexer_seq.txt", "w");
    export_token_seq(scanner_out, file_lexer_seq);
    fclose(file_lexer_seq);

    // --- 7. PARSER EXECUTION ---
    TreeNode* root = parser_skeleton(G, tables_info, scanner_out, 0, value_map);

    dynarray_destroy(scanner_out);
    destroy_tables(tables_info);
    free(value_map);
    dynadict_destroy(dict_map);

    if (root) {
        printf("\n--- Parse Tree ---\n");
        print_tree(root, "", true, true);
    }

    return 0;
}