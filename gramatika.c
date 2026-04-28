#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "dynarray.h"
#include "gramatika.h"
#include "scanner.h"
#include "hash.h"

#define BUFFER_SIZE_GRAMMAR 5000

void export_production(Production prod, char** index_mapping, FILE* out){
    fprintf(out, "[ %s -> ", index_mapping[prod.alpha]);
    for(int i = 0;i<dynarray_length(prod.beta);i++){
        fprintf(out, "%s ", index_mapping[prod.beta[i]]);
    }
    fprintf(out, "]\n", prod.alpha);
}

void print_production(Production prod, char** index_mapping){
    export_production(prod, index_mapping, stdout);
}

void export_build(BuildUp B, FILE* out) {
    switch(B.type) {
        case 0: // Shift
            fprintf(out, "SHIFT($%d)", B.BuildUnion.shbuild.shift_coord);
            break;
            
        case 1: // Append
            // Now printing both the source and the target list
            fprintf(out, "APPEND($%d -> $%d)", 
                    B.BuildUnion.apbuild.ap_from, 
                    B.BuildUnion.apbuild.ap_to);
            break;
            
        case 2: // Make Node
            fprintf(out, "MAKE(Class %d, Children: ", B.BuildUnion.mkbuild.classification);

            if(B.BuildUnion.mkbuild.coords == NULL){
                fprintf(out, "(NONE)");
                break;
            }

            int len = dynarray_length(B.BuildUnion.mkbuild.coords);
            for(int j = 0; j < len; j++) {
                fprintf(out, "$%d%s", 
                        B.BuildUnion.mkbuild.coords[j], 
                        (j < len - 1) ? ", " : "");
            }
            fprintf(out, ")");
            break;
            
        case 3: // Box
            fprintf(out, "BOX(ID %d)", B.BuildUnion.identifier);
            break;
            
        case 4: // Pass Int (Constant)
            fprintf(out, "VALUE(%d)", B.BuildUnion.identifier);
            break;
            
        default:
            fprintf(out, "UNKNOWN TYPE");
    }

    fprintf(out, "\n");
}

void print_build(BuildUp B, FILE* out){
    export_build(B, stdout);
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
        fprintf(out, "%3d | ", i);
        export_production(G.productions[i], index_mapping, out);
        fprintf(out, "    | ", i);
        export_build(G.builds[i], out);
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
    G.builds = dynarray_create(BuildUp);
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


void destroy_production(Production* production){
    dynarray_destroy(production->beta);
}

void destroy_buildUp(BuildUp* build){
    if(build->type == MAKE_NODE && build->BuildUnion.mkbuild.coords != NULL){
        dynarray_destroy(build->BuildUnion.mkbuild.coords);
    }
}


void destroy_grammar(Grammar* G){
    dynarray_destroy(G->T);
    dynarray_destroy(G->NT);
    for(int i = 0;i<dynarray_length(G->productions);i++){
        destroy_production(&(G->productions[i]));
    }
    for(int i = 0;i<dynarray_length(G->builds);i++){
        destroy_buildUp(&(G->builds[i]));
    }
    dynarray_destroy(G->productions);
}

char** storage_table_from_mapping(Pair* mapping, int map_size){
    char** inverse_map = malloc(map_size * sizeof(char*));
    for(int i=0;i<map_size;i++){
        inverse_map[i] = mapping[i].key;
    }

    return inverse_map;
}

bool coords_are_equal(int* a, int* b) {
    if (a == b) return true; // Same pointer? Identical.
    if (!a || !b) return false; // One is null, the other isn't.

    int len_a = dynarray_length(a);
    int len_b = dynarray_length(b);

    if (len_a != len_b) return false;

    for (int i = 0; i < len_a; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Main function to compare two lists of BuildUp structs
bool buildup_lists_are_equal(BuildUp* list1, BuildUp* list2) {
    if (list1 == list2) return true;
    if (!list1 || !list2) return false;

    int len1 = dynarray_length(list1);
    int len2 = dynarray_length(list2);

    if (len1 != len2) return false;

    int rule_num = 0;
    for (int i = 0; i < len1; i++) {
        BuildUp* b1 = &list1[i];
        BuildUp* b2 = &list2[i];

        //printf("RULE->%d\n", rule_num);
        // 1. Check type first
        if (b1->type != b2->type) return false;

        // 2. Deep compare the specific union member
        switch (b1->type) {
            case MAKE_NODE: // MakeBuild
                if (b1->BuildUnion.mkbuild.classification != b2->BuildUnion.mkbuild.classification)
                    //return false;
                // DEEP COPY CHECK: Compare the values inside the coords arrays
                if (!coords_are_equal(b1->BuildUnion.mkbuild.coords, b2->BuildUnion.mkbuild.coords))
                    return false;
                break;

            case APPEND: // AppendBuild
                if (b1->BuildUnion.apbuild.ap_from != b2->BuildUnion.apbuild.ap_from ||
                    b1->BuildUnion.apbuild.ap_to   != b2->BuildUnion.apbuild.ap_to)
                    return false;
                break;

            case SHIFT: // ShiftBuild
                if (b1->BuildUnion.shbuild.shift_coord != b2->BuildUnion.shbuild.shift_coord)
                    return false;
                break;

            case VALUE: // ValueBuild
                if (b1->BuildUnion.identifier != b2->BuildUnion.identifier)
                    return false;
                break;

            case BOX_NODE: // BoxBuild
                if (b1->BuildUnion.identifier != b2->BuildUnion.identifier)
                    return false;
                break;
                
            default:
                // Handle unknown types if necessary
                return false;
        }

        rule_num ++;
    }

    return true;
}

Grammar build_grammar(TableDFA rules_regex, char *file_lexing_rules, Hash dict_mapping, int symbols_amount, FILE* out, Pair** pair_ptr, char *** value_src){
    int ignore_categories[] = {1};
    Token* token_anchor = file_scan(rules_regex, file_lexing_rules, BUFFER_SIZE_GRAMMAR, ignore_categories, 1, "lexer/logs/rules/muncher_grammar.txt", "lexer/logs/rules/token_list.txt");
    // THIS SEQUENCE NEEDS TO BE DESTROYED AFTER THE PAIRS ARE NO LONGER NEEDED
    Token* token = token_anchor;

    export_token_seq(token, out);
    print_token_seq(token);

    Grammar G = create_grammar();
    Subset non_terminals_ss = SS_initialize_empty(symbols_amount);

    enum BuilderStates state = BASE_STATE;
    int head_word_class;
    int* beta_memory = dynarray_create(int);
    bool first_creation = true;

    Hash ast_map = dynadict_create(512, int);
    Pair* ast_pairs = dynarray_create(Pair);
    int pairs_count = 0;
    int token_count = 1;

    int* build_args = dynarray_create(int);
    bool empty_args = false;
    int build_type = -1;
    int build_integer = -1;
    int build_class = -1;

    while(token->category != 0){
        //printf("state-> %d category %d word-> %s\n", state, token->category, token->word);
        if(state == BASE_STATE && token->category == 2){
            int* pointer_get = dynadict_get(dict_mapping,token->word);
            //printf("word: %s \n", token->word);
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
        else if(state == BASE_STATE && token->category == 3){
            state = ALPHA_STATE;
        }
        else if(state == ALPHA_STATE && token->category == 4){
            state = BETA_STATE;
        }
        else if(state == BETA_STATE && token->category == 2){
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
            state = BETA_STATE;
        }
        else if(state == ARGS_STATE && token->category == 5){
            Production item = create_production(head_word_class, beta_memory, dynarray_length(beta_memory));
            BuildUp build;

            build.type = build_type;
            switch (build_type) {
            case SHIFT:
                assert(dynarray_length(build_args) == 1);
                assert(empty_args == false);
                assert(build_integer == -1);
                assert(build_class == -1);
                build.BuildUnion.shbuild.shift_coord = build_args[0];

                dynarray_reset(build_args);
                break;
            case APPEND:
                assert(dynarray_length(build_args) == 2);
                assert(empty_args == false);
                assert(build_integer == -1);
                assert(build_class == -1);
                build.BuildUnion.apbuild.ap_to = build_args[0];
                build.BuildUnion.apbuild.ap_from = build_args[1];

                dynarray_reset(build_args);
                break;
            case MAKE_NODE:
                assert((dynarray_length(build_args) > 0) ^ empty_args);
                assert(build_integer == -1);
                assert(build_class != -1);

                build.BuildUnion.mkbuild.classification = build_class;

                if(empty_args){
                    build.BuildUnion.mkbuild.coords = NULL;
                    dynarray_reset(build_args);
                }
                else{
                    build.BuildUnion.mkbuild.coords = build_args;
                    build_args = dynarray_create(int);
                }
                
                break;
            case BOX_NODE:
                assert(dynarray_length(build_args) == 0);
                assert(empty_args == false);
                assert(build_integer != -1);
                assert(build_class == -1);

                build.BuildUnion.identifier = build_integer;
                dynarray_reset(build_args);
                break;
            case VALUE:
                assert(dynarray_length(build_args) == 0);
                assert(empty_args == false);
                assert(build_integer != -1);
                assert(build_class == -1);

                build.BuildUnion.identifier = build_integer;
                dynarray_reset(build_args);
                break;
            default:
                assert(false);
            }

            dynarray_push(G.productions, item);
            beta_memory = dynarray_create(int);
            
            dynarray_push(G.builds, build);
            empty_args = false;
            build_integer = -1;
            build_class = -1;

            state = BASE_STATE;
        }
        else if(state == BETA_STATE && token->category == 6){
            state = BUILD_STATE;
        }
        else if(state == BUILD_STATE && token->category == 7){
            //printf("SHIFT\n");
            build_type = SHIFT;
            state = ARGS_STATE;
        }
        else if(state == BUILD_STATE && token->category == 8){
            //printf("Append\n");
            build_type = APPEND;
            state = ARGS_STATE;
        }
        else if(state == BUILD_STATE && token->category == 9){
            //printf("Make Node\n");
            build_type = MAKE_NODE;
            state = ARGS_STATE;
        }
        else if(state == BUILD_STATE && token->category == 10){
            //printf("Box Node\n");
            build_type = BOX_NODE;
            state = ARGS_STATE;
        }
        else if(state == BUILD_STATE && token->category == 11){
            //printf("Value\n");
            build_type = VALUE;
            state = ARGS_STATE;
        }
        else if(state == ARGS_STATE && token->category == 12){
            assert(token->word[0] == '-');
            assert(token->word[1] == '$');

            int num = atoi(&token->word[2]);
            dynarray_push(build_args, num);

            state = ARGS_STATE;
        }
        else if(state == ARGS_STATE && token->category == 13){
            empty_args = true;
            state = ARGS_STATE;
        }
        else if(state == ARGS_STATE && token->category == 14){
            int original_size = dynarray_length(token->word);
            int new_size = dynarray_length(token->word)-3;
            char* embedded_word = malloc(sizeof(char)*new_size+1);
            memcpy(embedded_word, &token->word[1], sizeof(char)*new_size);
            embedded_word[new_size] = '\0';
            
            //printf("WORD -> %s\n", embedded_word);
            if(!dynadict_add(ast_map, embedded_word, pairs_count)){
                //printf("NOT IN!\n");
                Pair new_pair = {embedded_word, pairs_count};
                dynarray_push(ast_pairs, new_pair);
                build_class = pairs_count;

                pairs_count ++;
            }
            else{
                //printf("ALREADY IN!\n");
                int* pointer_get = dynadict_get(ast_map, embedded_word);
                if(pointer_get == NULL){
                    printf("Token Not Found: %s\n", embedded_word);
                }
                assert(pointer_get != NULL);

                build_class = *pointer_get;
                free(embedded_word);
            }
            
            state = ARGS_STATE;
        }
        else if(state == ARGS_STATE && token->category == 15){
            int litnum = atoi(&token->word[1]);
            build_integer = litnum;

            state = ARGS_STATE;
        }

        else{
            printf("Rules Synthax Error State->%d Category->%d Word->%s Rule -> %d\n", state, token->category, token->word, token_count);
            assert(false);
        }

        token_count ++;
        token ++;
    }

    //printf("ARE BUILD THE SAME? -> %d\n", buildup_lists_are_equal(G.builds, builds));

    *pair_ptr = ast_pairs;
    *value_src = storage_table_from_mapping(ast_pairs, dynarray_length(ast_pairs));

    Subset terminals_ss = SS_deep_copy(non_terminals_ss);
    SS_inv(terminals_ss);
    
    G.NT = SS_to_list_indexes(non_terminals_ss);
    G.T = SS_to_list_indexes(terminals_ss);

    SS_destroy(&non_terminals_ss);
    SS_destroy(&terminals_ss);

    //dynarray_destroy(ast_pairs);
    
    dynadict_destroy(ast_map);
    // THIS ONLY DESTROYS DYNARRAY NOT SEQUENCE STRINGS WITHIN
    // NEED TO ITERATE
    dynarray_destroy(token_anchor);
    dynarray_destroy(beta_memory);
    dynarray_destroy(build_args);

    // AST PAIRS NEEDS TO BE USED AND DISCARDED LATER
    return G;
}