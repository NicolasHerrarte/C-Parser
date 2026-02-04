#include <stdlib.h>
#include <string.h> 
#include <stdio.h>

#include "subset.h"


#define ALT_PRIORITY 0
#define CONCAT_PRIORITY 1
#define CLOS_PRIORITY 2
#define FIN_PRIORITY 3
#define MAX_PRIORITY 4

#define EPSILON '@'

#define len_nfa_states(fa) dynarray_length(fa.states)

typedef struct AcceptableState{
    int state;
    int category;
}  AcceptableState;

typedef struct Transition{
    int state_from;
    int state_to;
    char trans_char;
} Transition;

typedef struct FA{
    int* states;
    int initial_state;
    bool alphabet[256];
    Transition* transitions;
    AcceptableState* acceptable_states;
} FA;

typedef struct Token{
    char* word;
    int category;
} Token;

typedef struct Fragment{
    int start_index;
    int end_index;
} Fragment;

void print_transition(Transition t);
void FA_print(FA fa);
void print_token_seq(Token* tokens);

void export_transition(Transition t, FILE* out);
void FA_export(FA fa, FILE* out);
void export_token_seq(Token* tokens, FILE* out);

void states_print(int* states);

int FA_initialize(FA *fa);
int FA_next_state(FA *fa);
bool int_dynarray_in(int* arr, int search);
bool FA_valid_state(FA fa, int state_check);
bool FA_fast_valid_state(FA fa, int state_check);
void FA_add_acceptable_state(FA *fa, int acceptable_state, int category);
bool FA_state_is_acceptable(FA fa, int state);
int acceptable_states_mapping(char* c);

Transition NFA_add_transition(FA *nfa, int _from, int _to, char _trans_char);
Transition DFA_add_transition(FA *dfa, int _from, int _to, char _trans_char);

int altercation(Fragment *left_fragment, Fragment *right_fragment, int *priority, int depth, int i);
int concatenation(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, int depth, int i, char next_char, char prev_char);
int closure(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, bool *final_split, int depth, bool split_found, int pass_priority, int i);
int fin_type(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, bool *final_split, char* state_identifier, char next_char, int depth, int i);
int parenthesis(Fragment fragment, Fragment *left_fragment, bool *final_split, bool split_found, int i);

void print_safe_char(char c);
Fragment find_split_point(FA* nfa, char* str, Fragment fragment, int final_state, bool recursion, bool debug);

Subset e_closure(FA nfa, Subset states_closure);
Subset delta(FA nfa, Subset q, char c);

int* NFA_transition_function(FA nfa, int state, char c);
int DFA_transition_function(FA dfa, int state, char c);

FA NtoDFA(FA nfa);
Token* scanner_loop_file(FA dfa, char* directory, int* ignore_cats, int amount_ignore);
Token* scanner_loop_string(FA dfa, char* src, int* ignore_cats, int amount_ignore);

FA MakeFA(char *src, char* out_dir, bool debug);

