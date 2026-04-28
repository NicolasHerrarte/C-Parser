#include <stdlib.h>
#include <string.h> 
#include <stdio.h>

#include "subset.h"


#define ALT_PRIORITY 0
#define CONCAT_PRIORITY 1
#define CLOS_PRIORITY 2
#define FIN_PRIORITY 3
#define MAX_PRIORITY 4

#define EPSILON '\x01'

#define len_nfa_states(fa) dynarray_length(fa.states)

#define INVALID 0
#define BAD -1

typedef struct ScannerState{
    int input;
    int fence;
    char* buffer;
    bool rollback;
} ScannerState;

typedef struct TableDFA{
    int** trans_table;
    int* acc_states;
    int* char_mapping;
    int num_states;
    int alphabet_size;
} TableDFA;

typedef struct ItemLexeme{
    int state;
    int pos;
} ItemLexeme;

typedef struct AcceptableState{
    int state;
    int category;
}  AcceptableState;

typedef struct Transition{
    int state_from;
    int state_to;
    char trans_char;
    bool epsilon_trans;
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
void TableDFA_export(TableDFA table, FILE* out);

void states_print(int* states);

int FA_initialize(FA *fa);
void FA_destroy(FA *fa);
int FA_next_state(FA *fa);
bool int_dynarray_in(int* arr, int search);
bool FA_valid_state(FA fa, int state_check);
bool FA_fast_valid_state(FA fa, int state_check);
void FA_add_acceptable_state(FA *fa, int acceptable_state, int category);
bool FA_state_is_acceptable(FA fa, int state);
int acceptable_states_mapping(char* c);

Transition NFA_add_transition(FA *nfa, int _from, int _to, char _trans_char, bool empty_trans);
Transition DFA_add_transition(FA *dfa, int _from, int _to, char _trans_char);

int altercation(Fragment *left_fragment, Fragment *right_fragment, int *priority, int depth, int i);
int concatenation(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, int depth, int i, char next_char, char prev_char);
int closure(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, bool *final_split, int depth, bool split_found, int pass_priority, int i);
int fin_type(Fragment fragment, Fragment *left_fragment, Fragment *right_fragment, int *priority, bool *final_split, char* state_identifier, char next_char, int depth, int i);
int parenthesis(Fragment fragment, Fragment *left_fragment, bool *final_split, bool split_found, int i);

void print_safe_char(char c);
Fragment find_split_point(FA* nfa, char* str, Fragment fragment, int final_state, bool recursion, FILE* split_out);

void e_closure(FA nfa, Subset* states_closure);
Subset delta(FA nfa, Subset q, char c);

int* NFA_transition_function(FA nfa, int state, char c);
int DFA_transition_function(FA dfa, int state, char c);

FA NtoDFA(FA nfa);
void saveDFATable(TableDFA tables, char* directory);
TableDFA loadDFATable(char* directory);
void destroyDFATable(TableDFA table);
long stream_len(FILE *stream);

void export_buffer(char* buffer, int input, int fence, int n, FILE* out);
Token next_word(TableDFA table, FILE* file_ptr, bool** failed_table, int* input_pos, ScannerState* sc, int n);
Token* file_scan(TableDFA table, char* directory, int buffer_size, int* ignore_cats, int amount_ignore, char* muncher_dir, char* finished_scan_dir);

TableDFA make_tables(char *src, char* save_table_dir, char* regex_logs_dir, char* split_logs_dir, char* nfa_logs_dir, char* dfa_logs_dir, char* table_logs_dir, bool debug);
void destroy_token_sequence(Token* sequence);

