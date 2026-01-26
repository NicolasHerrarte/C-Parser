#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "dynarray.h"
#include "re_pp.h"
#include "subset.h"
#include "scanner.h"

void print_safe_char(char c) {
    switch (c) {
        case '\n': printf("\\n"); break;
        case '\t': printf("\\t"); break;
        case '\r': printf("\\r"); break;
        case ' ':  printf("[SPC]"); break; // Or " " if you prefer
        default:   printf("%c", c);   break;
    }
}

void print_transition(Transition t){
    printf("State: ");
    printf("%d ", t.state_from);
    printf("- ");
    print_safe_char(t.trans_char);
    printf("-> State: ");
    printf("%d", t.state_to);
    printf("\n");
}

void FA_print(FA fa){
    printf("-- States --\n");
    for(int i = 0; i < dynarray_length(fa.states);i++){
        printf("%d, ", fa.states[i]);
    }
    printf("\n");
    printf("-- Acceptable States --\n");
    for(int i = 0; i < dynarray_length(fa.acceptable_states);i++){
        printf("(%d-%d), ", fa.acceptable_states[i].state, fa.acceptable_states[i].category);
    }
    printf("\n");
    printf("-- Alphabet --\n");
    for(int i = 0; i < 256;i++){
        if(fa.alphabet[i] == true){
            print_safe_char((unsigned char) i);
        }
        
    }
    printf("\n");
    //printf("-- Transitions --\n");
    for(int i = 0; i < dynarray_length(fa.transitions);i++){
        //print_transition(fa.transitions[i]);
    }

    printf("-- Starting State --\n");
    printf("- %d\n", fa.initial_state);
}

void states_print(int* states){
    printf("[ ");
    for(int i = 0;i<dynarray_length(states);i++){
        printf("%d, ", states[i]);
    }
    printf("]\n");
}

int FA_initialize(FA *fa){
    fa->states = dynarray_create(int);
    fa->transitions = dynarray_create(Transition);
    fa->acceptable_states = dynarray_create(AcceptableState);
    memset(fa->alphabet, 0, 256);
}

int FA_next_state(FA *fa){
    int next_int = dynarray_length(fa->states);
    dynarray_push(fa->states, next_int);

    return next_int;
}

bool int_dynarray_in(int* arr, int search){
    for(int i = 0;i<dynarray_length(arr);i++){
        if(arr[i] == search){
            return true;
        }
    }
    return false;
}


bool FA_valid_state(FA fa, int state_check){
    return int_dynarray_in(fa.states, state_check);
}

// Assumes states are ordered and without gaps: state[0] = 0, state[1] = 1, state[2] = 2 ...
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
        return error_fragment;
    }

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

Subset e_closure(FA nfa, Subset states_closure){
    int* states = SS_to_list_indexes(states_closure);
    int* inspect_states = dynarray_create(int);

    for(int i = 0;i<states_closure.count;i++){
        dynarray_push(inspect_states, states[i]);
    }

    while(dynarray_length(inspect_states) > 0){
        int n;

        dynarray_pop(inspect_states, &n);
        for(int i = 0;i<dynarray_length(nfa.transitions);i++){
            if(nfa.transitions[i].state_from == n && nfa.transitions[i].trans_char == EPSILON){
                if(!SS_in(states_closure, nfa.transitions[i].state_to)){
                    dynarray_push(inspect_states, nfa.transitions[i].state_to);
                    SS_add(&states_closure, nfa.transitions[i].state_to);
                }
            }
        }
    }

    dynarray_destroy(inspect_states);

    return states_closure;
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

    Subset n0 = SS_initialize(len_nfa_states(nfa), &nfa.initial_state, 1);
    Subset q0 = e_closure(nfa, n0);
    
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
            Subset t = e_closure(nfa, delta(nfa, q, c));

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

    return dfa;
}

Token* scanner_loop_file(FA dfa, char* directory, int* ignore_cats, int amount_ignore){
    FILE* file_ptr = fopen(directory, "r");

    int current_state = dfa.initial_state;
    int last_acceptable_state = -1;

    char* curr_word = dynarray_create(char);

    Token* token_list = dynarray_create(Token);

    if(file_ptr == NULL){
        return 0;
    }

    int c_int;
    while((c_int = fgetc(file_ptr)) != EOF){

        char c = (char) c_int;

        int next_state = DFA_transition_function(dfa, current_state, c);
    
        //printf("%c %d\n", c, next_state);
        
        if(next_state == -1){
            if(last_acceptable_state != -1){
                Token t;
                char null_token = '\0';
                dynarray_push(curr_word, null_token);
                t.word = curr_word;
                
                for(int i = 0;i<dynarray_length(dfa.acceptable_states);i++){
                    if(dfa.acceptable_states[i].state == last_acceptable_state){
                        t.category = dfa.acceptable_states[i].category;
                        break;
                    };
                }

                //printf("%s, %d\n", t.word, t.category);
                bool is_ignore = false;
                for(int i=0;i<amount_ignore;i++){
                    if(ignore_cats[i]==t.category){
                        is_ignore = true;
                    }
                }

                if(!is_ignore){
                    dynarray_push(token_list, t);
                }

                current_state = DFA_transition_function(dfa, dfa.initial_state, c);
                last_acceptable_state = -1;
                if(FA_state_is_acceptable(dfa, current_state)){
                    last_acceptable_state = current_state;
                }
                curr_word = dynarray_create(char);
                dynarray_push(curr_word, c);
            }
            else{
                printf("\nLexer Compilation Error\n");
                break;
            }
        }
        else{
            dynarray_push(curr_word, c);
            current_state = next_state;
            if(FA_state_is_acceptable(dfa, current_state)){
                last_acceptable_state = current_state;
            }
        }
         
    }

    if(last_acceptable_state != -1){
        Token t;
        char null_token = '\0';
        dynarray_push(curr_word, null_token);
        t.word = curr_word;
        
        for(int i = 0;i<dynarray_length(dfa.acceptable_states);i++){
            if(dfa.acceptable_states[i].state == last_acceptable_state){
                t.category = dfa.acceptable_states[i].category;
                break;
            };
        }

        bool is_ignore = false;
        for(int i=0;i<amount_ignore;i++){
            if(ignore_cats[i]==t.category){
                is_ignore = true;
            }
        }

        if(!is_ignore){
            dynarray_push(token_list, t);
        }

        Token final_token;
        final_token.word = "";
        final_token.category = 0;

        dynarray_push(token_list, final_token);
    }
    else{
        printf("\nLexer Compilation Error\n");
    }

    return token_list;
}

Token* scanner_loop_string(FA dfa, char* src, int* ignore_cats, int amount_ignore){
    int current_state = dfa.initial_state;
    int last_acceptable_state = -1;

    char* curr_word = dynarray_create(char);

    Token* token_list = dynarray_create(Token);

    if(src == NULL){
        return 0;
    }

    int src_i = 0;
    while(src[src_i] != '\0'){
        char c = src[src_i];

        int next_state = DFA_transition_function(dfa, current_state, c);
    
        //printf("%c %d\n", c, next_state);
        
        if(next_state == -1){
            if(last_acceptable_state != -1){
                Token t;
                char null_token = '\0';
                dynarray_push(curr_word, null_token);
                t.word = curr_word;
                
                for(int i = 0;i<dynarray_length(dfa.acceptable_states);i++){
                    if(dfa.acceptable_states[i].state == last_acceptable_state){
                        t.category = dfa.acceptable_states[i].category;
                        break;
                    };
                }

                //printf("%s, %d\n", t.word, t.category);
                
                bool is_ignore = false;
                for(int i=0;i<amount_ignore;i++){
                    if(ignore_cats[i]==t.category){
                        is_ignore = true;
                    }
                }

                if(!is_ignore){
                    dynarray_push(token_list, t);
                }

                current_state = DFA_transition_function(dfa, dfa.initial_state, c);
                last_acceptable_state = -1;
                if(FA_state_is_acceptable(dfa, current_state)){
                    last_acceptable_state = current_state;
                }
                curr_word = dynarray_create(char);
                dynarray_push(curr_word, c);
            }
            else{
                printf("\nLexer Compilation Error\n");
                break;
            }
        }
        else{
            dynarray_push(curr_word, c);
            current_state = next_state;
            if(FA_state_is_acceptable(dfa, current_state)){
                last_acceptable_state = current_state;
            }
        }
        
        src_i++;
    }

    if(last_acceptable_state != -1){
        Token t;
        char null_token = '\0';
        dynarray_push(curr_word, null_token);
        t.word = curr_word;
        
        for(int i = 0;i<dynarray_length(dfa.acceptable_states);i++){
            if(dfa.acceptable_states[i].state == last_acceptable_state){
                t.category = dfa.acceptable_states[i].category;
                break;
            };
        }

        //printf("\n%s, %d\n", t.word, t.category);
        bool is_ignore = false;
        for(int i=0;i<amount_ignore;i++){
            if(ignore_cats[i]==t.category){
                is_ignore = true;
            }
        }

        if(!is_ignore){
            dynarray_push(token_list, t);
        }

        Token final_token;
        final_token.word = "";
        final_token.category = 0;

        dynarray_push(token_list, final_token);
    }
    else{
        printf("\nLexer Compilation Error\n");
    }

    return token_list;
}

FA MakeFA(char *src, bool debug){
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

    return dfa;
}