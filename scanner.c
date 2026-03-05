#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "dynarray.h"
#include "re_pp.h"
#include "subset.h"
#include "scanner.h"


void export_safe_char(char c, FILE* out) {
    switch (c) {
        case '\n': fprintf(out, "\\n"); break;
        case '\t': fprintf(out, "\\t"); break;
        case '\r': fprintf(out, "\\r"); break;
        case ' ':  fprintf(out, "[SPC]"); break; // Or " " if you prefer
        default:   fprintf(out, "%c", c);   break;
    }
}

void print_safe_char(char c) {
    export_safe_char(c, stdout);
}

void print_transition(Transition t){
    export_transition(t, stdout);
}

void export_transition(Transition t, FILE* out){
    fprintf(out, "State: ");
    fprintf(out, "%d ", t.state_from);
    fprintf(out, "- ");
    export_safe_char(t.trans_char, out);
    fprintf(out, "-> State: ");
    fprintf(out, "%d", t.state_to);
    fprintf(out, "\n");
}

void FA_export(FA fa, FILE* out){
    fprintf(out, "-- States --\n");
    for(int i = 0; i < dynarray_length(fa.states);i++){
        fprintf(out, "%d, ", fa.states[i]);
    }
    fprintf(out, "\n");
    fprintf(out, "-- Acceptable States --\n");
    for(int i = 0; i < dynarray_length(fa.acceptable_states);i++){
        fprintf(out, "(%d-%d), ", fa.acceptable_states[i].state, fa.acceptable_states[i].category);
    }
    fprintf(out, "\n");
    fprintf(out, "-- Alphabet --\n");
    for(int i = 0; i < 256;i++){
        if(fa.alphabet[i] == true){
            export_safe_char((unsigned char) i, out);
        }
        
    }
    fprintf(out, "\n");
    //printf("-- Transitions --\n");
    for(int i = 0; i < dynarray_length(fa.transitions);i++){
        export_transition(fa.transitions[i], out);
    }

    fprintf(out, "-- Starting State --\n");
    fprintf(out, "- %d\n", fa.initial_state);
}

void FA_print(FA fa){
    FA_export(fa, stdout);
}

void states_print(int* states){
    printf("[ ");
    for(int i = 0;i<dynarray_length(states);i++){
        printf("%d, ", states[i]);
    }
    printf("]\n");
}

void print_token_seq(Token* tokens){
    export_token_seq(tokens, stdout);
}

void export_token_seq(Token* tokens, FILE* out){
    for(int i = 0;i<dynarray_length(tokens);i++){
        fprintf(out, "(%s, %d)\n", tokens[i].word, tokens[i].category);
    }
}

int FA_next_state(FA *fa){
    int next_int = dynarray_length(fa->states);
    dynarray_push(fa->states, next_int);

    return next_int;
}

int FA_initialize(FA *fa){
    fa->states = dynarray_create(int);
    fa->transitions = dynarray_create(Transition);
    fa->acceptable_states = dynarray_create(AcceptableState);
    memset(fa->alphabet, 0, 256);
}

bool int_dynarray_in(int* arr, int search){
    for(int i = 0;i<dynarray_length(arr);i++){
        if(arr[i] == search){
            return true;
        }
    }
    return false;
}

void FA_destroy(FA *fa){
    dynarray_destroy(fa->states);
    dynarray_destroy(fa->transitions);
    dynarray_destroy(fa->acceptable_states);
}

bool FA_valid_state(FA fa, int state_check){
    return int_dynarray_in(fa.states, state_check);
}

bool FA_fast_valid_state(FA fa, int state_check){
    return (state_check < dynarray_length(fa.states));
}

void FA_add_acceptable_state(FA *fa, int acceptable_state, int category){
    assert(FA_valid_state(*fa, acceptable_state));
    AcceptableState acc_state;
    acc_state.state = acceptable_state;
    acc_state.category = category;
    dynarray_push(fa->acceptable_states, acc_state);
}

bool FA_state_is_acceptable(FA fa, int state){
    for(int i = 0;i<dynarray_length(fa.acceptable_states);i++){
        if(fa.acceptable_states[i].state == state){
            return true;
        }
    }
    return false;
}

int acceptable_states_mapping(char* c){
    return atoi(c);
}

Transition NFA_add_transition(FA *nfa, int _from, int _to, char _trans_char){
    assert(FA_valid_state(*nfa, _from));
    assert(FA_valid_state(*nfa, _to));
    Transition trans;
    trans.state_from = _from;
    trans.state_to = _to;
    trans.trans_char = _trans_char;
    dynarray_push(nfa->transitions, trans);

    if(_trans_char != EPSILON){
        int int_repr = (unsigned char) _trans_char;
        nfa->alphabet[int_repr] = 1;
    }

    return trans;
}

Transition DFA_add_transition(FA *dfa, int _from, int _to, char _trans_char){
    assert(FA_valid_state(*dfa, _from));
    assert(FA_valid_state(*dfa, _to));
    Transition trans;
    trans.state_from = _from;
    trans.state_to = _to;
    trans.trans_char = _trans_char;

    for(int i = 0;i<dynarray_length(dfa->transitions);i++){
        assert(!(dfa->transitions[i].state_from == _from && dfa->transitions[i].trans_char == _trans_char));
    }
    dynarray_push(dfa->transitions, trans);

    if(_trans_char != EPSILON){
        int int_repr = (unsigned char) _trans_char;
        dfa->alphabet[int_repr] = 1;
    }

    return trans;
}


int altercation(Fragment *left_fragment, Fragment *right_fragment, int *priority, int depth, int i){
    if(ALT_PRIORITY < *priority && depth == 0){
        left_fragment->end_index = i;
        right_fragment->start_index = i+1;
        *priority = ALT_PRIORITY;
        return true;
    }
    return false;
}

int concatenation(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, int depth, int i, char next_char, char prev_char){
    if(CONCAT_PRIORITY < *priority && depth == 0){
        if(i >= fragment.start_index+1){
            left_fragment->end_index = i;
            right_fragment->start_index = i;
            *priority = CONCAT_PRIORITY;
            return true;
        }
        //im really unsure about this but lets roll
        //else{
            //if(next_char != '\0' && next_char != '*' && next_char != '$'){
                //left_fragment->end_index = i+1;
                //right_fragment->start_index = i+1;
                //*priority = CONCAT_PRIORITY;
                //return true;
            //} 
        //}
    }
    return false;
}

int closure(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, bool *final_split, int depth, bool split_found, int pass_priority, int i){
    if(pass_priority <= *priority && depth == 0 && i == fragment.end_index-1){
        left_fragment->end_index = i;
        *final_split = true;
        *priority = pass_priority;
        return true;
    }
    return false;
}

int fin_type(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, bool *final_split, char* state_identifier, char next_char, int depth, int i){
    if(FIN_PRIORITY <= *priority && depth == 0 && i == fragment.end_index-3){
        left_fragment->end_index = i;
        *final_split = true;
        *priority = FIN_PRIORITY;
        *state_identifier = next_char;
        return true;
    }
    return false;
}

int parenthesis(Fragment fragment, Fragment *left_fragment, bool *final_split, bool split_found, int i){
    if(i == fragment.end_index-1 && split_found == false){
        left_fragment->start_index += 1;
        left_fragment->end_index = i;
        *final_split = true;
        return true;
    }
    return false;
}

Fragment find_split_point(FA* nfa, char* str, Fragment fragment, int final_state, bool recursion, bool debug){
    Fragment left_fragment = {fragment.start_index, fragment.start_index};
    Fragment right_fragment = {fragment.end_index, fragment.end_index};

    int min_priority = MAX_PRIORITY;
    int parenthesis_depth = 0;

    bool final_split = false;
    bool split_found = false;
    bool found_solution = false;

    char acc_state_identifier = '\0';

    if(debug){
        for(int i = fragment.start_index;i<fragment.end_index;i++){
            print_safe_char(str[i]);
        }
        printf("\n");
    }

    if(fragment.start_index == fragment.end_index-1){
        int state_head = FA_next_state(nfa);
        int state_tail = FA_next_state(nfa);

        if(final_state > 0){
            FA_add_acceptable_state(nfa, state_tail, final_state);
        }
        Transition trans = NFA_add_transition(nfa, state_head, state_tail, str[fragment.start_index]);
        Fragment char_fragment = {state_head, state_tail};

        nfa->initial_state = state_head;
        return char_fragment;
    }
    else if(fragment.start_index == fragment.end_index-2 && str[fragment.start_index] == '/'){
        int state_head = FA_next_state(nfa);
        int state_tail = FA_next_state(nfa);

        if(final_state > 0){
            FA_add_acceptable_state(nfa, state_tail, final_state);
        }
        Transition trans = NFA_add_transition(nfa, state_head, state_tail, str[fragment.start_index+1]);
        Fragment char_fragment = {state_head, state_tail};

        nfa->initial_state = state_head;
        return char_fragment;
    }
    else{
        for(int i = fragment.start_index;i<fragment.end_index;i++){
            char current_char = str[i];
            char previous_char;
            char following_char;
            if(i == 0){
                previous_char = '\0';
            }
            else{
                previous_char = str[i-1];
            }

            if(i == fragment.end_index-1){
                following_char = '\0';
            }
            else{
                following_char = str[i+1];
            }

            if(current_char == '|'){
                found_solution = altercation(&left_fragment, &right_fragment, &min_priority, parenthesis_depth, i);
            }
            else if(current_char == '*'){
                found_solution = closure(fragment, &left_fragment, &right_fragment, &min_priority, &final_split, parenthesis_depth, split_found, CLOS_PRIORITY, i);
            }
            else if(current_char == '$'){
                found_solution = fin_type(fragment, &left_fragment, &right_fragment, &min_priority, &final_split, &acc_state_identifier, following_char, parenthesis_depth, i);
                i += 2;
            }
            else if(current_char == '('){
                found_solution = concatenation(fragment, &left_fragment, &right_fragment, &min_priority, parenthesis_depth, i, '\0', previous_char);
                parenthesis_depth += 1;
            }
            else if(current_char == ')'){
                parenthesis_depth -= 1;
                found_solution = parenthesis(fragment, &left_fragment, &final_split, split_found, i);
            }
            else if(current_char == '/'){
                found_solution = concatenation(fragment, &left_fragment, &right_fragment, &min_priority, parenthesis_depth, i, following_char, previous_char);
                i++;
            }
            else{
                found_solution = concatenation(fragment, &left_fragment, &right_fragment, &min_priority, parenthesis_depth, i, following_char, previous_char);
            }

            if(found_solution > 0){
                split_found = true;
            }
    }
    }
    
    if (parenthesis_depth != 0){
        printf("Parenthesis Mismatch -> %d", parenthesis_depth);
        Fragment error_fragment = {0, 0};
    }

    assert(parenthesis_depth == 0);

    //printf("(%d, %d) (%d, %d)\n",left_fragment.start_index, left_fragment.end_index, right_fragment.start_index, left_fragment.end_index);

    if(recursion == true){
        if(final_split == true){
            switch(min_priority){
                case CLOS_PRIORITY:
                    Fragment nfa_only_fragment = find_split_point(nfa, str, left_fragment, 0, true, debug);

                    int state_head_alt = FA_next_state(nfa);
                    int state_tail_alt = FA_next_state(nfa);
                    
                    Transition trans_start = NFA_add_transition(nfa, state_head_alt, nfa_only_fragment.start_index, EPSILON);
                    Transition trans_tail = NFA_add_transition(nfa, nfa_only_fragment.end_index, state_tail_alt, EPSILON);
                    Transition trans_ret = NFA_add_transition(nfa, nfa_only_fragment.end_index, nfa_only_fragment.start_index, EPSILON);
                    Transition trans_bounce = NFA_add_transition(nfa, state_head_alt, state_tail_alt, EPSILON);

                    Fragment alt_fragment = {state_head_alt, state_tail_alt};

                    if(final_state > 0){
                        FA_add_acceptable_state(nfa, state_tail_alt, final_state);
                    }

                    nfa->initial_state = state_head_alt;
                    return alt_fragment;
                case FIN_PRIORITY:
                    char char_identifier[3];

                    char_identifier[0] = str[left_fragment.end_index+1];
                    char_identifier[1] = str[left_fragment.end_index+2];
                    char_identifier[2] = '\0';
                    int num_state_identifier = acceptable_states_mapping(char_identifier);
                    Fragment final_tag_fragment = find_split_point(nfa, str, left_fragment, num_state_identifier, true, debug);
                    return final_tag_fragment;
                case MAX_PRIORITY:
                    Fragment same_fragment = find_split_point(nfa, str, left_fragment, 0, true, debug);
                    if(final_state > 0){
                        FA_add_acceptable_state(nfa, same_fragment.end_index, final_state);
                    }
                    return same_fragment;
                default:
                    printf("Something is not right");
            }
        }
        else{
            Fragment nfa_left_fragment = find_split_point(nfa, str, left_fragment, 0, true, debug);
            Fragment nfa_right_fragment = find_split_point(nfa, str, right_fragment, 0, true, debug);

            switch(min_priority){
                case ALT_PRIORITY:
                    //printf("ALTERCATION\n");
                    int state_head_alt = FA_next_state(nfa);
                    int state_tail_alt = FA_next_state(nfa);

                    Transition trans_0 = NFA_add_transition(nfa, state_head_alt, nfa_left_fragment.start_index, EPSILON);
                    Transition trans_1 = NFA_add_transition(nfa, state_head_alt, nfa_right_fragment.start_index, EPSILON);
                    Transition trans_2 = NFA_add_transition(nfa, nfa_left_fragment.end_index, state_tail_alt, EPSILON);
                    Transition trans_3 = NFA_add_transition(nfa, nfa_right_fragment.end_index, state_tail_alt, EPSILON);

                    Fragment alt_fragment = {state_head_alt, state_tail_alt};

                    if(final_state > 0){
                        FA_add_acceptable_state(nfa, state_tail_alt, final_state);
                    }

                    nfa->initial_state = state_head_alt;
                    return alt_fragment;
                    
                case CONCAT_PRIORITY:
                    //printf("CONCATENATION\n");
                    
                    Transition trans = NFA_add_transition(nfa, nfa_left_fragment.end_index, nfa_right_fragment.start_index, EPSILON);
                    Fragment concat_fragment = {nfa_left_fragment.start_index, nfa_right_fragment.end_index};

                    if(final_state > 0){
                        FA_add_acceptable_state(nfa, nfa_right_fragment.end_index, final_state);
                    }

                    nfa->initial_state = nfa_left_fragment.start_index;
                    return concat_fragment;

                default:
                    printf("Something is not right\n");
            }
        }
    }
}

// Because delta creates a subset from scratch there is no memory leak and no need to make a deep copy
void e_closure(FA nfa, Subset* states_closure){
    int* states = SS_to_list_indexes(*states_closure);
    int* inspect_states = dynarray_create(int);

    for(int i = 0;i<states_closure->count;i++){
        dynarray_push(inspect_states, states[i]);
    }

    while(dynarray_length(inspect_states) > 0){
        int n;

        dynarray_pop(inspect_states, &n);
        for(int i = 0;i<dynarray_length(nfa.transitions);i++){
            if(nfa.transitions[i].state_from == n && nfa.transitions[i].trans_char == EPSILON){
                if(!SS_in(*states_closure, nfa.transitions[i].state_to)){
                    dynarray_push(inspect_states, nfa.transitions[i].state_to);
                    SS_add(states_closure, nfa.transitions[i].state_to);
                }
            }
        }
    }

    dynarray_destroy(inspect_states);
    dynarray_destroy(states);
}

int* NFA_transition_function(FA nfa, int state, char c){
    int* out_transitions = dynarray_create(int);
    for(int i = 0;i<dynarray_length(nfa.transitions);i++){
        if(nfa.transitions[i].state_from == state && nfa.transitions[i].trans_char == c){
            dynarray_push(out_transitions, nfa.transitions[i].state_to);
        }
    }

    return out_transitions;
}

int DFA_transition_function(FA dfa, int state, char c){
    int out_transition = -1;
    for(int i = 0;i<dynarray_length(dfa.transitions);i++){
        if(dfa.transitions[i].state_from == state && dfa.transitions[i].trans_char == c){
            out_transition = dfa.transitions[i].state_to;
        }
    }

    return out_transition;
}

Subset delta(FA nfa, Subset q, char c){
    Subset delta_out = SS_initialize_empty(len_nfa_states(nfa));
    int* q_list = SS_to_list_indexes(q);
    for(int i = 0;i<q.count;i++){
        int* state_transitions = NFA_transition_function(nfa, q_list[i], c);
        for(int j = 0;j<dynarray_length(state_transitions);j++){
            SS_add(&delta_out, state_transitions[j]);
        }

        dynarray_destroy(state_transitions);
    }

    dynarray_destroy(q_list);

    return delta_out;
}

FA NtoDFA(FA nfa){

    int alphabet_length = 0;
    for(int i = 0;i<256;i++){
        if(nfa.alphabet[i] == true){
            alphabet_length++;
        }
    }

    char* alphabet_list = char_b_table_to_list(nfa.alphabet);

    Subset q0 = SS_initialize(len_nfa_states(nfa), &nfa.initial_state, 1);
    e_closure(nfa, &q0);

    //Subset n0 = SS_initialize(len_nfa_states(nfa), &nfa.initial_state, 1);
    //Subset q0 = e_closure(nfa, n0);
    
    Subset* Q = dynarray_create(Subset);
    Subset** T = dynarray_create(Subset*);
    Subset* worklist = dynarray_create(Subset);

    dynarray_push(Q, q0);
    dynarray_push(worklist, q0);

    while(dynarray_length(worklist) > 0){
        Subset q;
        dynarray_pop(worklist, &q);

        //SSS_print(q);

        Subset* q_slot = malloc(alphabet_length * sizeof(Subset));
        dynarray_push(T, q_slot);

        for(int i = 0;i<alphabet_length; i++){
            char c = alphabet_list[i];

            Subset t = delta(nfa, q, c);
            e_closure(nfa, &t);
            //Subset t = e_closure(nfa, delta(nfa, q, c));

            q_slot[i] = t;
            //printf("Char %c\n", c);
            //SSS_print(t);

            if(t.count > 0 && !SS_list_in(Q, t)){
                //printf("Add\n");
                dynarray_push(Q, t);
                dynarray_pushleft(worklist, t);
            }
        }
    }

    dynarray_destroy(worklist);

    FA dfa;
    FA_initialize(&dfa);
    
    for(int i = 0;i<dynarray_length(Q);i++){
        int next_state = FA_next_state(&dfa);
    }

    for(int i = 0;i<dynarray_length(Q);i++){
        int max_priority = 0;
        bool is_accepting_state = false;

        //SSS_print(Q[i]);
        
        for(int j = 0;j<dynarray_length(nfa.acceptable_states);j++){
            if(SS_in(Q[i], nfa.acceptable_states[j].state)){
                is_accepting_state = true;
                if(nfa.acceptable_states[j].category > max_priority){
                    max_priority = nfa.acceptable_states[j].category;
                }
            }
        }
        if(is_accepting_state == true){
            FA_add_acceptable_state(&dfa, i, max_priority);
        }
    }

    dfa.initial_state = 0;
    memcpy(dfa.alphabet, nfa.alphabet, sizeof(bool[256]));
    for(int i = 0;i<dynarray_length(T);i++){

        for(int j = 0;j<alphabet_length;j++){
            //printf("%d, %c\n", i, alphabet_list[j]);
            //SSS_print(T[i][j]);
            int d_index = SS_list_index(Q, T[i][j]);
            if(d_index != -1){
                DFA_add_transition(&dfa, i, d_index, alphabet_list[j]);
            }
        }
    }

    for(int i = 0;i<dynarray_length(T);i++){
        for(int j = 0;j<alphabet_length;j++){
            SS_destroy(&(T[i][j]));
        }
        free(T[i]);
    }

    dynarray_destroy(T);
    dynarray_destroy(Q);

    return dfa;
}

TableDFA DFAtoTable(FA dfa){
    unsigned char char_list[256] = {0};
    int* char_mapping = malloc(256*sizeof(int));
    memset(char_mapping, -1, 256*sizeof(int));
    int counter = 0;
    for(int i = 0;i<256;i++){
        if(dfa.alphabet[i] == true){
            char_list[counter] = i;
            char_mapping[i] = counter;
            counter ++;
        }
    }

    int** scanner_table = malloc(sizeof(int*)*counter);
    for(int i = 0;i<counter;i++){
        scanner_table[i] = malloc((dynarray_length(dfa.states)+1)* sizeof(int));
        memset(scanner_table[i], 0, (dynarray_length(dfa.states)+1)* sizeof(int));
    }

    for(int i = 0;i<dynarray_length(dfa.transitions);i++){
        Transition trans = dfa.transitions[i];
        scanner_table[char_mapping[trans.trans_char]][trans.state_from+1] = trans.state_to+1;
    }
    
    int* acceptable_table = calloc(dynarray_length(dfa.states)+1, sizeof(int));
    for(int i = 0;i<dynarray_length(dfa.acceptable_states);i++){
       acceptable_table[dfa.acceptable_states[i].state+1] = dfa.acceptable_states[i].category;
    }

    TableDFA tout;
    tout.trans_table = scanner_table;
    tout.acc_states = acceptable_table;
    tout.char_mapping = char_mapping;
    tout.num_states = dynarray_length(dfa.states)+1;
    tout.alphabet_size = counter;
    return tout;
}

void saveDFATable(TableDFA tables, char* directory){
    FILE* f = fopen(directory, "w");
    assert(f != NULL);

    fprintf(f, "%d,%d,\n\n", tables.num_states, tables.alphabet_size);

    for (int c = 0; c < 256; c++) {
        for (int col = 0; col < tables.alphabet_size; col++) {
            if (tables.char_mapping[c] == col) {
                fprintf(f, "%d,", c);
            }
        }
    }

    fprintf(f, "\n");

    for (int row = 0; row < tables.num_states; row++) {
        for (int col = 0; col < tables.alphabet_size; col++) {
            fprintf(f, "%d,", tables.trans_table[col][row]);
        }

        fprintf(f, "\n");
    }

    fprintf(f, "\n");

    for (int col = 0; col < tables.num_states; col++) {
        fprintf(f, "%d,", tables.acc_states[col]);
    }

    fclose(f);
}

TableDFA loadDFATable(char* directory){
    TableDFA tables;
    FILE* f = fopen(directory, "r");
    assert(f != NULL);

    fscanf(f, " %d, %d,", &tables.num_states, &tables.alphabet_size);

    //printf("%d, %d\n", tables.num_states, tables.alphabet_size);

    int* char_mapping = malloc(256*sizeof(int));
    memset(char_mapping, -1, 256*sizeof(int));
    int** scanner_table = malloc(sizeof(int*)*tables.alphabet_size);
    for(int i = 0;i<tables.alphabet_size;i++){
        scanner_table[i] = malloc(tables.num_states* sizeof(int));
        memset(scanner_table[i], 0, tables.num_states* sizeof(int));
    }

    int* acceptable_table = calloc(tables.num_states, sizeof(int));

    for(int i = 0;i<tables.alphabet_size;i++){
        int c_int;
        fscanf(f, " %d,", &c_int);
        char_mapping[c_int] = i;
    }

    //printf("%d, %d, %d\n", char_mapping['a'], char_mapping['b'], char_mapping['c']);

    for(int i = 0;i<tables.num_states;i++){
        for(int j = 0;j<tables.alphabet_size;j++){
            fscanf(f, " %d,", &scanner_table[j][i]);
            //printf("%d (%d, %d)\n", scanner_table[j][i]);
        }
    }

    for(int i = 0;i<tables.num_states;i++){
        fscanf(f, " %d,", &acceptable_table[i]);
        //printf("%d\n", acceptable_table[i]);
    }

    fclose(f);

    tables.acc_states = acceptable_table;
    tables.trans_table = scanner_table;
    tables.char_mapping = char_mapping;

    return tables;
}

void destroyDFATable(TableDFA table){
    free(table.acc_states);
    free(table.char_mapping);
    for(int i = 0;i<table.alphabet_size;i++){
        free(table.trans_table[i]);
    }
    free(table.trans_table);
}

void printTableDFA(TableDFA table) {
    printf("\n--- DFA TRANSITION TABLE ---\n");
    
    printf("State | Acc |");
    for (int c = 0; c < 256; c++) {
        for (int col = 0; col < table.alphabet_size; col++) {
            if (table.char_mapping[c] == col) {
                if (c > 32 && c < 127) printf("  %c  ", c);
                else printf(" 0x%02X", c);
            }
        }
    }
    printf("\n");
    printf("----------------------------\n");

    for (int s = 0; s < table.num_states; s++) {
        printf("%5d | %3d |", s, table.acc_states[s]);

        for (int col = 0; col < table.alphabet_size; col++) {
            int target = table.trans_table[col][s];
            if (target == 0) {
                printf("  .  ");
            } else {
                printf("%3d  ", target);
            }
        }
        printf("\n");
    }
    printf("----------------------------\n");
}

long stream_len(FILE *stream) {
    long count = 0;
    int c;

    while ((c = fgetc(stream)) != EOF) {
        if (c == '\r') {
            continue; 
        }
        count++;
    }

    rewind(stream);

    return count;
}


Token next_word(TableDFA table, FILE* file_ptr, bool** failed_table, int* input_pos, ScannerState* sc, int n){
    int state = 1;
    char* lexeme = dynarray_create(char);
    ItemLexeme* stack = dynarray_create(ItemLexeme);

    ItemLexeme start = {BAD, BAD};
    dynarray_push(stack, start);

    while(state != INVALID){
        //char c = fgetc(file_ptr);
        //NextChar
        int c = sc->buffer[sc->input];
        //printf("Current Char %c\n", c);
        //printf("Start State %d\n", state);
        sc->input = (sc->input + 1) % (2*n);
        if(sc->input % n == 0){
            for(int i = 0; i < n; i++) {
                int next_c = getc(file_ptr);
                sc->buffer[sc->input + i] = (next_c == EOF) ? '\0' : (char) next_c;
            }

            sc->fence = (sc->input+n) % (2*n);
        }
        //NextChar
        dynarray_push(lexeme, c);

        //printf("POS -> %d\n", *input_pos);
        if(failed_table[state][*input_pos]){
            printf("Table FUCK UP\n");
            break;
        }

        if(table.acc_states[state] > 0){
            dynarray_reset(stack);
        }

        ItemLexeme next = {state, *input_pos};

        dynarray_push(stack, next);

        if(c == '\0'){
            char garbage;
            dynarray_pop(lexeme, &garbage);
            break;
        }

        if(table.char_mapping[c] == -1){
            printf("Character Not Recognized\n");
            assert(false);
        }
        state = table.trans_table[table.char_mapping[c]][state];

        (*input_pos) ++;

        //printf("End State %d\n", state);
    }

    //for(int i = 0;i<dynarray_length(stack);i++){
        //break;
        //printf("Stack: %d Pos: %d\n", stack[i].state, stack[i].pos);
    //}

    //printf("State -> %d\n", state);

    while(state != BAD && table.acc_states[state] == 0){
        //printf("OMFG %d, %d\n", state, sc->fence);
        failed_table[state][*input_pos] = true;

        //printf("FK %d\n", dynarray_length(stack));
        assert(dynarray_length(stack)>=0);
        ItemLexeme tmp;
        dynarray_pop(stack, &tmp);
        state = tmp.state;
        //printf("%d, %d\n", state);
        *input_pos = tmp.pos;

        char garbage;
        dynarray_pop(lexeme, &garbage);
        //Rollback()
        if(sc->input == sc->fence){
            printf("Rollback error %c\n", garbage);
            assert(false);
        }
        sc->input = (sc->input - 1 + (2 * n)) % (2 * n);
        //Rollback()
    }

    dynarray_destroy(stack);

    //printf("DONE LOOP\n");

    //printf("state -> %d\n", state);
    if(state != BAD && table.acc_states[state] > 0){
        //printf("%d\n", dynarray_length(lexeme));
        char null_char = '\0';
        dynarray_push(lexeme, null_char);
        //printf("%s -> %d\n", lexeme, state);

        Token next_token = {lexeme, table.acc_states[state]};
        return next_token;
    }
    else{
        printf("Lexing Error\n");
        assert(false);
    }
}

Token* file_scan(TableDFA table, char* directory, int buffer_size, int* ignore_cats, int amount_ignore){
    FILE* file_ptr = fopen(directory, "r");
    Token* token_list = dynarray_create(Token);

    ScannerState sc_state;
    sc_state.fence = 0;
    sc_state.input = 0;
    sc_state.buffer = malloc(buffer_size * 2 * sizeof(char));

    for(int i = 0; i < buffer_size; i++){
        int next_c = getc(file_ptr);
        sc_state.buffer[i] = (next_c == EOF) ? '\0' : (char) next_c;
    }

    rewind(file_ptr);
    long file_size = stream_len(file_ptr)+2;
    int input_pos = 0;
    
    bool** failed_table = malloc(table.num_states*sizeof(bool*));
    for(int i = 0;i<table.num_states;i++){
        failed_table[i] = calloc(file_size, sizeof(bool));
    }
    //printf("SIZE --- %d\n", file_size);

    while(input_pos < file_size-2){
        Token token = next_word(table, file_ptr, failed_table, &input_pos, &sc_state, buffer_size);
        //printf("str: %s, cat: %d input: %d\n", token.word, token.category, input_pos);
        bool is_ignore = false;
        for(int i=0;i<amount_ignore;i++){
            if(ignore_cats[i]==token.category){
                is_ignore = true;
            }
        }

        if(!is_ignore){
            dynarray_push(token_list, token);
        }
    }

    Token final_token;
    final_token.category = 0;
    final_token.word = "EOF";
    
    dynarray_push(token_list, final_token);
    //print_token_seq(token_list);
    //printf("input: %d, state: %d, fence: %d\n", input_pos, sc_state.input, sc_state.fence);
    for(int i = 0;i<table.num_states;i++){
        free(failed_table[i]);
    }
    free(failed_table);
    free(sc_state.buffer);
    fclose(file_ptr);

    return token_list;
}

TableDFA make_tables(char *src, char* out_dir, char* save_dir, bool debug){
    if(debug){
        printf("\ninitializing non finite automata...\n");
    }
    FA nfa;
    FA_initialize(&nfa);
    if(debug){
        printf("\npreprocessign regex...\n\n");
    }
    char* regex = regex_prep(src);
    Fragment fragment_start = {0, strlen(regex)};

    if(debug){
        printf("Processsed Regex -> \n");
        printf("%s\n", regex);
        printf("\ncreating thomson's construction...\n\n");
    }

    find_split_point(&nfa, regex, fragment_start, false, true, debug);

    if(debug){
        printf("\nNFA -> \n");
        FA_print(nfa);
        printf("\nsubset creation to definite finite automata...\n\n");
    }
    FA dfa = NtoDFA(nfa);

    if(debug){
        printf("DFA -> \n");
        FA_print(dfa);
    }

    FILE* out = fopen(out_dir, "w");

    fprintf(out, "--- Post Regex ---\n");
    fprintf(out, "%s\n", regex);
    fprintf(out, "\nNFA -> \n");
    FA_export(nfa, out);
    fprintf(out, "\nDFA -> \n");
    FA_export(dfa, out);
    fclose(out);

    FA_destroy(&nfa);
    dynarray_destroy(regex);

    TableDFA table_construct = DFAtoTable(dfa);
    FA_destroy(&dfa);
    saveDFATable(table_construct, save_dir);

    return table_construct;
}