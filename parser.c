#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "dynarray.h"
#include "subset.h"

enum {
    END,
    EPSILON,
    GOAL,
    EXPR,
    TERM,
    FACTOR,
    EXPR_ALPHA,
    TERM_ALPHA,
    PLUS,
    MINUS,
    MULT,
    DIV,
    NUM,
    NAME,
    LEFT_PAR,
    RIGHT_PAR,
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
                    rhs = SS_union(rhs, temp_rhs);
                    k++;
                }   
            }
            if(k==prod_beta_size-1 && SS_in(first[B[k]], EPSILON)){
                SS_add(&rhs, EPSILON);
            }
            Subset first_A_tmp = SS_union(first[A], rhs);
            if(!SS_equal(first[A], first_A_tmp)){
                SS_print(first_A_tmp);
                SS_print(first[A]);
                changed = true;
            }

            first[A] = first_A_tmp;
            
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
        printf("---\n");
        for(int PUTA = 0;PUTA<dynarray_length(s);PUTA++){
            print_item(s[PUTA]);
        }
        change = false;
        for(int i = 0;i<dynarray_length(s);i++){
            int curr_k = s[i].k;
            int C = s[i].beta[curr_k];
            int beta_length = dynarray_length(s[i].beta);
            for(int j = 0;j<dynarray_length(G.productions);j++){
                if(G.productions[j].alpha == C){    
                    if(curr_k+1<beta_length){
                        int delta = s[i].beta[curr_k+1];
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
        counter++;
    } while(change == true);

    return s;
}

Item* goto_table(Grammar G, Item* s_raw, int x){
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

    return moved;
}

int main() {
    printf("Parser...\n");

    // Make a way for this initialization to be made only with rules and string not this bs
    Grammar G = create_grammar();
    int terminals[] = {PLUS, MINUS, MULT, DIV, LEFT_PAR, RIGHT_PAR, NUM, NAME, EPSILON, END};
    int non_terminals[] = {GOAL, EXPR, EXPR_ALPHA, TERM, TERM_ALPHA, FACTOR};

    G.S = GOAL;

    int b0[] = {EXPR};
    Production item0 = create_production(GOAL, b0, 1);
    dynarray_push(G.productions, item0);

    int b1[] = {TERM, EXPR_ALPHA};
    Production item1 = create_production(EXPR, b1, 2);
    dynarray_push(G.productions, item1);

    int b2[] = {PLUS, TERM, EXPR_ALPHA};
    Production item2 = create_production(EXPR_ALPHA, b2, 3);
    dynarray_push(G.productions, item2);

    int b3[] = {MINUS, TERM, EXPR_ALPHA};
    Production item3 = create_production(EXPR_ALPHA, b3, 3);
    dynarray_push(G.productions, item3);

    int b4[] = {EPSILON};
    Production item4 = create_production(EXPR_ALPHA, b4, 1);
    dynarray_push(G.productions, item4);

    int b5[] = {FACTOR, TERM_ALPHA};
    Production item5 = create_production(TERM, b5, 2);
    dynarray_push(G.productions, item5);

    int b6[] = {MULT, FACTOR, TERM_ALPHA};
    Production item6 = create_production(TERM_ALPHA, b6, 3);
    dynarray_push(G.productions, item6);

    int b7[] = {DIV, FACTOR, TERM_ALPHA};
    Production item7 = create_production(TERM_ALPHA, b7, 3);
    dynarray_push(G.productions, item7);

    int b8[] = {EPSILON};
    Production item8 = create_production(TERM_ALPHA, b8, 1);
    dynarray_push(G.productions, item8);

    int b9[] = {LEFT_PAR, EXPR, RIGHT_PAR};
    Production item9 = create_production(FACTOR, b9, 3);
    dynarray_push(G.productions, item9);

    int b10[] = {NUM};
    Production item10 = create_production(FACTOR, b10, 1);
    dynarray_push(G.productions, item10);

    int b11[] = {NAME};
    Production item11 = create_production(FACTOR, b11, 1);
    dynarray_push(G.productions, item11);

    for(int i = 0;i<10;i++){
        int t = terminals[i];
        dynarray_push(G.T, t);
    }
    for(int i = 0;i<6;i++){
        int nt = non_terminals[i];
        dynarray_push(G.NT, nt);
    }

    print_grammar(G);

    Subset* first = generate_first(G);
    for(int i = 0;i<16;i++){
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
    int tfuck = EXPR;
    dynarray_push(initial_item.beta, tfuck);
    initial_item.lookahead = END;
    initial_item.k = 0;

    Item* s = dynarray_create(Item);
    dynarray_push(s, initial_item);

    printf("What\n");
    printf("%d\n", dynarray_length(s));
    Item* c = closure(G, s, first);

    //for(int i = 0;i<dynarray_length(c);i++){
        //print_item(c[i]);
    //}
}