#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "scanner.h"
#include "tree.h"
#include "ast.h"
#include "gramatika.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define DEFAULT_STACK_SIZE 4
#define BUFFER_SIZE 500

enum {
    END,
    EPSILON_P,
    GOAL,
};

enum {
    TREENODE_POS,
    AST_POS,
    TOKEN_POS,
    STATE_POS,
};

typedef struct ParserOutput{
    TreeNode* CST;
    TreeManager AST;
} ParserOutput;

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

// From Scanner
typedef struct LRTransition{
    int state_from;
    int state_to;
    int trans_symbol;
} LRTransition;

typedef union StackItem{
    Token token;
    ASTNode ast_node;
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

void assert_table_mappings_equal(TableMapping* a, TableMapping* b);

void print_transition_single(LRTransition t, char** symbol_names);
void print_transition_list(LRTransition* transitions, char** symbol_names);
void print_transitions(LRTransition* transitions, int count, char** symbol_names, int num_terminals);
void print_item(Item item, char** index_mapping, int max_alpha, int max_rhs);
void print_item_list(Item* c, char** val_table, char* title);
void print_first_sets(Grammar G, Subset* first, char** val_table);
void print_canonical_collection(CC_Item* CC, char** val_table);
void print_tables(TableMapping* tm);
void print_stack(StackItem* stack, char** index_mapping);

int get_rhs_width(Item item, char** index_mapping);
void export_item(Item item, char** index_mapping, int max_alpha, int max_rhs, FILE* out);
void export_transition_single(LRTransition t, char** symbol_names, FILE* out);
void export_transition_list(LRTransition* transitions, char** symbol_names, FILE* out);
void export_item_list(Item* c, char** val_table, char* title, FILE* out);
void export_first_sets(Grammar G, Subset* first, char** val_table, FILE* out);
void export_canonical_collection(CC_Item* CC, char** val_table, FILE* out);
void export_tables(TableMapping* tm, FILE* out);
void export_stack(StackItem* stack, char** index_mapping, FILE* out);
void export_handle(Production prod, char** index_mapping, FILE* out);

bool item_equal(Item item1, Item item2);
bool item_in(Item* items, Item find_item);
Item item_copy(Item original);
Item* item_list_copy(Item* original);

uint64_t hash_item(void* item_ptr);
bool hash_item_equal(void* a, void* b);
uint64_t hash_item_list(void* items_ptr);
bool hash_item_list_equal(void* a_ptr, void* b_ptr);
uint64_t hash_CC_item(void* CC_ptr);
bool hash_CC_item_equal(void* a_ptr, void* b_ptr);
bool int_equal(void* a, void* b) ;

Hash* dictionary_from_mapping(Pair* mapping, int map_size);

Subset* generate_first(Grammar G);
Item* item_closure(Grammar G, Item* s_raw, Subset* first);
Item* goto_table(Grammar G, Item* s, Subset* first, int x);
TableMaterial c_collection(Grammar G, Subset* first);
TableMapping create_tables(Grammar G, TableMaterial tb);
int get_stack_position(int stack_len, int element, int offset);
ParserOutput parser_skeleton(Grammar G, TableMapping tb, Token* token_ptr, char** index_mapping, FILE* skeleton_out);

void save_parsing_tables(TableMapping* tm, char* directory);
TableMapping load_parsing_tables(char* directory);

void destroy_first(Grammar G, Subset* first);
void destroy_tables(TableMapping t_mapping);

TableMapping tables_pipeline(Grammar G, Pair* mapping, int symbols_amount, char* prod_rules_src, char** value_map, char* save_table_dir, char* parser_logs_dir, bool debug);
TreeManager parse_pipeline(char* language_src, char* language_regex, char* rules_src, char* rules_regex, int* ignore_cats_language, int* ignore_cats_rules, Pair* mapping, int symbols_amount, char* lexer_dir, char* parser_dir, bool generate_parsing_tables, bool generate_lexing_tables, bool debug);