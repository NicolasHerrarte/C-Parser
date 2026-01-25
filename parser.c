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

// Scanner Output
typedef struct Token{
    char* word;
    int category;
} Token;

typedef union StackItem{
    Token token;
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

void print_transition(LRTransition t){
    printf("State: ");
    printf("%d ", t.state_from);
    printf("- ");
    printf("%d ", t.trans_symbol);
    printf("-> State: ");
    printf("%d", t.state_to);
    printf("\n");
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

void print_item(Item item){
    printf("[ %d -> ", item.alpha);
    for(int i = 0;i<dynarray_length(*item.beta);i++){
        if(i != -1 && i == item.k){
            printf("*");
        }
        printf("%d ", (*item.beta)[i]);
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
                        if(delta == EPSILON){
                            continue;
                        }
                        found_delta_first = true;

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

                        dynarray_destroy(delta_first_list);
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

    return closure(G, moved, first);
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
    cc0.cc = closure(G, s, first);
    cc0.marked = false;

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

    TableMaterial fout;
    fout.CC = CC;
    fout.goto_transitions = trans;
    return fout;
}

void print_tables(TableMapping* tm) {
    if (tm == NULL) return;

    printf("\n--- LR(1) PARSER TABLES (Canonical Closure) ---\n\n");

    // 1. PRINT HEADER ROW (Symbols)
    // Using tm-> to access struct members
    printf("%-5s |", "State");
    for (int j = 0; j < tm->t_count; j++) {
        printf(" T%-3d |", tm->action_mapping[j]);
    }
    for (int j = 0; j < tm->nt_count; j++) {
        printf(" NT%-2d |", tm->goto_mapping[j]);
    }
    printf("\n");

    // 2. PRINT SEPARATOR
    int total_width = 7 + ((tm->t_count + tm->nt_count) * 8);
    for (int i = 0; i < total_width; i++) printf("-");
    printf("\n");

    // 3. PRINT TABLE ROWS
    for (int i = 0; i < tm->states_count; i++) {
        // Print Current State ID
        printf("%-5d |", i);

        // ACTION TABLE COLUMNS
        for (int j = 0; j < tm->t_count; j++) {
            int action_type = tm->table_action[i][j][0]; 
            int action_val  = tm->table_action[i][j][1];

            if (action_type == 2) {
                printf(" s%-3d |", action_val);
            } else if (action_type == 1) {
                printf(" acc  |");
            } else if (action_val == 3) {
                printf(" r%-3d |", action_type + 1); 
            } else {
                printf("      |");
            }
        }

        // GOTO TABLE COLUMNS
        for (int j = 0; j < tm->nt_count; j++) {
            int state_to = tm->table_goto[i][j];
            if (state_to != -1) {
                printf(" %-4d |", state_to);
            } else {
                printf("      |");
            }
        }
        printf("\n");
    }
    printf("\n");
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

    printf("--- Mappings ---\n");

    for(int i=0;i<dynarray_length(tb.goto_transitions);i++){
        int curr_symbol = tb.goto_transitions[i].trans_symbol;
        if(!SS_in(counted, curr_symbol)){
            if(SS_in(fast_terminal,curr_symbol)){
                symbols_mapping[curr_symbol] = t_count;
                dynarray_push(action_mapping, curr_symbol);
                printf("Terminal[%d] = %d\n", curr_symbol, t_count);
                t_count ++;
            }
            else{
                symbols_mapping[curr_symbol] = nt_count;
                dynarray_push(goto_mapping, curr_symbol);
                printf("Non-Terminal[%d] = %d\n", curr_symbol, nt_count);
                nt_count ++;
            }
            SS_add(&counted, curr_symbol);
        }
    }

    printf("--- Counts ---\n");
    printf("T: %d, NT: %d\n", t_count, nt_count);
    printf("--- Actions ---\n");

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
            printf("Action[i->%d, c->%d] = shift j->%d\n", curr_trans.state_from, curr_trans.trans_symbol, curr_trans.state_to);
            table_action[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]][0] = 2;
            table_action[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]][1] = curr_trans.state_to;
        }
        else{
            printf("Goto[i->%d, n->%d] = j->%d\n", curr_trans.state_from, curr_trans.trans_symbol, curr_trans.state_to);
            table_goto[curr_trans.state_from][symbols_mapping[curr_trans.trans_symbol]] = curr_trans.state_to;
        }
    }

    for(int i=0;i<dynarray_length(tb.CC);i++){
        for(int j=0;j<dynarray_length(tb.CC[i].cc);j++){
            Item curr_item = tb.CC[i].cc[j];
            int* curr_beta = *curr_item.beta;
            if(curr_item.k == dynarray_length(curr_beta)){
                if(curr_item.alpha == GOAL){
                    printf("Action[i->%d, a->%d] = acc\n", i, curr_item.lookahead);
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
                    printf("Action[i->%d, a->%d] = reduce p->%d\n", i, curr_item.lookahead, p_rule+1);
                    table_action[i][symbols_mapping[curr_item.lookahead]][0] = 3;
                    table_action[i][symbols_mapping[curr_item.lookahead]][1] = p_rule;
                }
            }
        }
    }
    
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

void print_stack(StackItem* stack) {
    int len = dynarray_length(stack);
    printf("[ ");
    
    for (int i = 0; i < len; i++) {
        // Even index (0, 2, 4...) -> It's a Symbol/Token
        if (i % 2 == 0) {
            printf("%d ", stack[i].token.category);
        } 
        // Odd index (1, 3, 5...) -> It's a State (int)
        else {
            printf("%d ", stack[i].s_int);
        }
    }
    
    printf("]\n");
}

void parser_skeleton(Grammar G, TableMapping tb, Token* token_ptr, int extra_parameters){
    StackItem* stack = dynarray_create_prealloc(StackItem,100);
    StackItem* token_bs = malloc((2+extra_parameters)*2*sizeof(StackItem));

    StackItem first_word;
    StackItem first_state;

    first_state.s_int = 0;

    first_word.token.word = "WHAT";
    first_word.token.category = END;

    dynarray_push(stack, first_word);
    dynarray_push(stack, first_state);

    //printf("STACK: %d\n", first_state.s_int);

    do{
        StackItem top_state = dynarray_get_last(stack);

        print_stack(stack);
        printf("Word %d\n", token_ptr->category);
        printf("State %d\n", top_state.s_int);
        //printf("Stack Top-> %d\n", top_state.s_int);
        //printf("Current Word-> %s\n", token_ptr->word);

        int word_category_table = tb.symbols_mapping[token_ptr->category];

        if(tb.table_action[top_state.s_int][word_category_table][0] == 3){

            int prod_rule = tb.table_action[top_state.s_int][word_category_table][1];
            int A = G.productions[prod_rule].alpha;
            int* beta = G.productions[prod_rule].beta;

            for(int i=0;i<(2+extra_parameters)*dynarray_length(beta);i++){
                dynarray_pop(stack, &token_bs[i]);
            }
            
            //printf("stack get %d\n", dynarray_get_last(stack).s_int);
            //printf("already_mapped %d\n", tb.symbols_mapping[A]);
            int to_state = tb.table_goto[dynarray_get_last(stack).s_int][tb.symbols_mapping[A]];

            StackItem new_token;
            StackItem new_state;

            new_token.token.word = "WHAT";
            new_token.token.category = A;

            new_state.s_int = to_state;
            
            dynarray_push(stack, new_token);
            dynarray_push(stack, new_state);
           
            
            printf("Reduce-> %d\n", prod_rule+1);
        }
        else if(tb.table_action[top_state.s_int][word_category_table][0] == 2){
            int to_state = tb.table_action[top_state.s_int][word_category_table][1];

            StackItem new_token;
            StackItem new_state;

            new_token.token.word = "WHAT";
            new_token.token.category = token_ptr->category;

            new_state.s_int = to_state;

            dynarray_push(stack, new_token);
            dynarray_push(stack, new_state);

            //printf("STACK: %d\n", new_state.s_int);

            token_ptr++;

            printf("Shift-> %d\n", to_state);
        }
        else if(tb.table_action[top_state.s_int][word_category_table][0] == 1){
            printf("Accept");
            break;
        }
        else{
            printf("Error");
            break;
        }

        //printf("%s", token_ptr->word);
    } while(true);
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
    int* master_beta = dynarray_create(int);
    initial_item.beta = &master_beta;
    dynarray_push_rval(*initial_item.beta, LIST);
    initial_item.lookahead = END;
    initial_item.k = 0;

    Item initial_item1;
    initial_item1.alpha = LIST;
    int* master_beta1 = dynarray_create(int);
    initial_item1.beta = &master_beta1;
    dynarray_push_rval(*initial_item1.beta, LIST);
    dynarray_push_rval(*initial_item1.beta, PAIR);
    initial_item1.lookahead = END;
    initial_item1.k = 1;

    Item initial_item2;
    initial_item2.alpha = LIST;
    int* master_beta2 = dynarray_create(int);
    initial_item2.beta = &master_beta2;
    dynarray_push_rval(*initial_item2.beta, LIST);
    dynarray_push_rval(*initial_item2.beta, PAIR);
    initial_item2.lookahead = LEFT_PAR;
    initial_item2.k = 1;

    Item* s = dynarray_create(Item);
    dynarray_push(s, initial_item);
    dynarray_push(s, initial_item1);
    dynarray_push(s, initial_item2);

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
    
    //printf("Moment of Truth\n");
    TableMaterial table_material = c_collection(G, first);
    //CC_Item* CC = table_material.CC;
    //LRTransition* trans = table_material.goto_transitions;

    //for(int i = 0;i<dynarray_length(CC);i++){
        //printf("---\n");
        //for(int j = 0;j<dynarray_length(CC[i].cc);j++){
            //print_item(CC[i].cc[j]);
        //}
        //printf("---\n");
    //}

    //for(int i = 0;i<dynarray_length(trans);i++){
        ///printf("---\n");
        //print_transition(trans[i]);
    //}

    TableMapping tables_info = create_tables(G, table_material);
    print_tables(&tables_info);

    Token* scanner_out = dynarray_create(Token);
    Token lpar;
    lpar.word = "(";
    lpar.category = 5;
    Token rpar;
    rpar.word = ")";
    rpar.category = 6;
    Token end_of_file;
    end_of_file.word = "\0";
    end_of_file.category = 0;

    dynarray_push(scanner_out, lpar);
    dynarray_push(scanner_out, lpar);
    dynarray_push(scanner_out, rpar);
    dynarray_push(scanner_out, lpar);
    dynarray_push(scanner_out, rpar);
    dynarray_push(scanner_out, rpar);
    dynarray_push(scanner_out, end_of_file);

    parser_skeleton(G, tables_info, scanner_out, 0);

    //Hash my_map = hash_create(5, Item*, hash_item_list);

    //printf("%llu", hash_item_list(c));

    //hash_add(my_map, g, hash_item_list_equal);
    //hash_add(my_map, g, hash_item_list_equal);
    //hash_add(my_map, c, hash_item_list_equal);
    //hash_add(my_map, s, hash_item_list_equal);


    //Item** tl = my_map.obj_storage;


    //printf("AAAA");
    //for(int i = 0;i<dynarray_length(tl);i++){
        //printf("---\n");
        //for(int j = 0;j<dynarray_length(tl[i]);j++){
            //print_item(tl[i][j]);
        //}
        //printf("---\n");
    //}
    
}