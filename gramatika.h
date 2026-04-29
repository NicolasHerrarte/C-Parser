#include "dynarray.h"
#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "hash.h"

enum Instructions {
    SHIFT,
    APPEND,
    MAKE_NODE,
    BOX_NODE,
    VALUE
};

enum BuilderStates{
    BASE_STATE,
    ALPHA_STATE,
    BETA_STATE,
    BUILD_STATE,
    ARGS_STATE,
};

typedef struct Production{
    int alpha;
    int* beta;
} Production;

typedef struct MakeBuild{
    int classification;
    int* coords;
} MakeBuild;

typedef struct AppendBuild{
    int ap_from;
    int ap_to;
} AppendBuild;

typedef struct ShiftBuild{
    int shift_coord;
} ShiftBuild;

typedef struct BuildUp{
    int type;
    union{
        MakeBuild mkbuild;
        AppendBuild apbuild;
        ShiftBuild shbuild;
        int identifier;
    } BuildUnion;
} BuildUp;

typedef struct Grammar{
    int* T;
    int* NT;
    int S;
    Production* productions;
    BuildUp* builds;
} Grammar;

typedef struct Pair{
    char* key;
    int value;
} Pair;

void export_production(Production prod, char** index_mapping, FILE* out);
void print_production(Production prod, char** index_mapping);
void export_build(BuildUp B, FILE* out);
void print_build(BuildUp B, FILE* out);
void export_grammar(Grammar G, char** index_mapping, FILE* out);
void print_grammar(Grammar G, char** index_mapping);
Grammar create_grammar();
Production create_production(int a, int* b, int b_count);
void destroy_production(Production* production);
void destroy_buildUp(BuildUp* build);
void destroy_grammar(Grammar* G);
Grammar build_grammar(TableDFA rules_regex, char *file_lexing_rules, Hash dict_mapping, int symbols_amount, Pair** pair_ptr, char *** value_src, char* rules_logs_dir, bool debug);

char** storage_table_from_mapping(Pair* mapping, int map_size);