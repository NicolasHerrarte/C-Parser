#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "dynarray.h"
#include "gramatika.h"
#include "scanner.h"
#include "hash.h"

#define BUFFER_SIZE_GRAMMAR 4096

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

Grammar build_grammar(TableDFA rules_regex, char *file_lexing_rules, Hash dict_mapping, int symbols_amount, FILE* out){
    int ignore_categories[] = {1};
    Token* token_anchor = file_scan(rules_regex, file_lexing_rules, BUFFER_SIZE_GRAMMAR, ignore_categories, 1);
    Token* token = token_anchor;

    BuildUp* builds = dynarray_create(BuildUp);

    BuildUp build_default;
    build_default.type = SHIFT;
    build_default.BuildUnion.shbuild.shift_coord = 0;

    BuildUp build_default_1;
    build_default_1.type = SHIFT;
    build_default_1.BuildUnion.shbuild.shift_coord = 1;

    BuildUp build4;
    build4.type = APPEND;
    build4.BuildUnion.apbuild.ap_from = 1;
    build4.BuildUnion.apbuild.ap_to = 0;

    BuildUp build3;
    build3.type = MAKE_NODE;
    int* build3_coords = dynarray_create(int);
    dynarray_push_rval(build3_coords, 0);
    build3.BuildUnion.mkbuild.coords = build3_coords;
    build3.BuildUnion.mkbuild.classification = 56;

    BuildUp build14;
    build14.type = MAKE_NODE;
    int* build14_coords = dynarray_create(int);
    dynarray_push_rval(build14_coords, 2);
    dynarray_push_rval(build14_coords, 4);
    build14.BuildUnion.mkbuild.coords = build14_coords;
    build14.BuildUnion.mkbuild.classification = 60;

    BuildUp build15;
    build15.type = MAKE_NODE;
    int* build15_coords = dynarray_create(int);
    dynarray_push_rval(build15_coords, 2);
    dynarray_push_rval(build15_coords, 4);
    dynarray_push_rval(build15_coords, 6);
    build15.BuildUnion.mkbuild.coords = build15_coords;
    build15.BuildUnion.mkbuild.classification = 60;

    BuildUp build16;
    build16.type = MAKE_NODE;
    int* build16_coords = dynarray_create(int);
    dynarray_push_rval(build16_coords, 2);
    dynarray_push_rval(build16_coords, 4);
    dynarray_push_rval(build16_coords, 6);
    build16.BuildUnion.mkbuild.coords = build16_coords;
    build16.BuildUnion.mkbuild.classification = 60;

    BuildUp build19;
    build19.type = MAKE_NODE;
    int* build19_coords = dynarray_create(int);
    dynarray_push_rval(build19_coords, 2);
    dynarray_push_rval(build19_coords, 4);
    build19.BuildUnion.mkbuild.coords = build19_coords;
    build19.BuildUnion.mkbuild.classification = 62;

    BuildUp build20;
    build20.type = MAKE_NODE;
    int* build20_coords = dynarray_create(int);
    dynarray_push_rval(build20_coords, 2);
    dynarray_push_rval(build20_coords, 4);
    dynarray_push_rval(build20_coords, 6);
    dynarray_push_rval(build20_coords, 8);
    build20.BuildUnion.mkbuild.coords = build20_coords;
    build20.BuildUnion.mkbuild.classification = 63;

    BuildUp build21;
    build21.type = MAKE_NODE;
    int* build21_coords = dynarray_create(int);
    dynarray_push_rval(build21_coords, 1);
    dynarray_push_rval(build21_coords, 2);
    build21.BuildUnion.mkbuild.coords = build21_coords;
    build21.BuildUnion.mkbuild.classification = 64;

    BuildUp build22;
    build22.type = MAKE_NODE;
    int* build22_coords = dynarray_create(int);
    dynarray_push_rval(build22_coords, 1);
    dynarray_push_rval(build22_coords, 2);
    dynarray_push_rval(build22_coords, 4);
    build22.BuildUnion.mkbuild.coords = build22_coords;
    build22.BuildUnion.mkbuild.classification = 64;

    BuildUp build23;
    build23.type = MAKE_NODE;
    int* build23_coords = dynarray_create(int);
    dynarray_push_rval(build23_coords, 1);
    dynarray_push_rval(build23_coords, 3);
    dynarray_push_rval(build23_coords, 6);
    dynarray_push_rval(build23_coords, 7);
    build23.BuildUnion.mkbuild.coords = build23_coords;
    build23.BuildUnion.mkbuild.classification = 65;

    BuildUp build24;
    build24.type = MAKE_NODE;
    int* build24_coords = dynarray_create(int);
    dynarray_push_rval(build24_coords, 0);
    dynarray_push_rval(build24_coords, 2);
    build24.BuildUnion.mkbuild.coords = build24_coords;
    build24.BuildUnion.mkbuild.classification = 65;

    BuildUp build25;
    build25.type = MAKE_NODE;
    int* build25_coords = dynarray_create(int);
    dynarray_push_rval(build25_coords, 0);
    build25.BuildUnion.mkbuild.coords = build25_coords;
    build25.BuildUnion.mkbuild.classification = 67;

    BuildUp build26;
    build26.type = MAKE_NODE;
    int* build26_coords = dynarray_create(int);
    dynarray_push_rval(build26_coords, 0);
    build26.BuildUnion.mkbuild.coords = build26_coords;
    build26.BuildUnion.mkbuild.classification = 100;

    BuildUp build27;
    build27.type = VALUE;
    build27.BuildUnion.identifier = 0;
    
    BuildUp build28;
    build28.type = VALUE;
    build28.BuildUnion.identifier = 1;

    BuildUp build29;
    build29.type = VALUE;
    build29.BuildUnion.identifier = 2;

    BuildUp build30;
    build30.type = VALUE;
    build30.BuildUnion.identifier = 3;

    BuildUp build31;
    build31.type = MAKE_NODE;
    build31.BuildUnion.mkbuild.coords = NULL;
    build31.BuildUnion.mkbuild.classification = 51;

    BuildUp build32;
    build32.type = MAKE_NODE;
    build32.BuildUnion.mkbuild.coords = NULL;
    build32.BuildUnion.mkbuild.classification = 52;

    BuildUp build33;
    build33.type = MAKE_NODE;
    build33.BuildUnion.mkbuild.coords = NULL;
    build33.BuildUnion.mkbuild.classification = 39;

    BuildUp build34;
    build34.type = MAKE_NODE;
    int* build34_coords = dynarray_create(int);
    dynarray_push_rval(build34_coords, 1);
    build34.BuildUnion.mkbuild.coords = build34_coords;
    build34.BuildUnion.mkbuild.classification = 39;

    BuildUp build35;
    build35.type = MAKE_NODE;
    int* build35_coords = dynarray_create(int);
    dynarray_push_rval(build35_coords, 1);
    build35.BuildUnion.mkbuild.coords = build35_coords;
    build35.BuildUnion.mkbuild.classification = 53;

    BuildUp build37;
    build37.type = MAKE_NODE;
    build37.BuildUnion.mkbuild.coords = NULL;
    build37.BuildUnion.mkbuild.classification = 32;

    BuildUp build38;
    build38.type = APPEND;
    build38.BuildUnion.apbuild.ap_from = 2;
    build38.BuildUnion.apbuild.ap_to = 0;

    BuildUp build39;
    build39.type = MAKE_NODE;
    int* build39_coords = dynarray_create(int);
    dynarray_push_rval(build39_coords, 0);
    build39.BuildUnion.mkbuild.coords = build39_coords;
    build39.BuildUnion.mkbuild.classification = 32;

    BuildUp build41;
    build41.type = MAKE_NODE;
    build41.BuildUnion.mkbuild.coords = NULL;
    build41.BuildUnion.mkbuild.classification = 70;    

    BuildUp build42;
    build42.type = APPEND;
    build42.BuildUnion.apbuild.ap_from = 2;
    build42.BuildUnion.apbuild.ap_to = 0;

    BuildUp build43;
    build43.type = MAKE_NODE;
    int* build43_coords = dynarray_create(int);
    dynarray_push_rval(build43_coords, 0);
    build43.BuildUnion.mkbuild.coords = build43_coords;
    build43.BuildUnion.mkbuild.classification = 70;

    BuildUp build44;
    build44.type = MAKE_NODE;
    int* build44_coords = dynarray_create(int);
    dynarray_push_rval(build44_coords, 0);
    dynarray_push_rval(build44_coords, 2);
    build44.BuildUnion.mkbuild.coords = build44_coords;
    build44.BuildUnion.mkbuild.classification = 72;

    BuildUp build45;
    build45.type = MAKE_NODE;
    int* build45_coords = dynarray_create(int);
    dynarray_push_rval(build45_coords, 0);
    dynarray_push_rval(build45_coords, 1);
    dynarray_push_rval(build45_coords, 2);
    build45.BuildUnion.mkbuild.coords = build45_coords;
    build45.BuildUnion.mkbuild.classification = 30;

    BuildUp build47;
    build47.type = MAKE_NODE;
    int* build47_coords = dynarray_create(int);
    dynarray_push_rval(build47_coords, 0);
    dynarray_push_rval(build47_coords, 2);
    build47.BuildUnion.mkbuild.coords = build47_coords;
    build47.BuildUnion.mkbuild.classification = 7;

    BuildUp build48;
    build48.type = MAKE_NODE;
    int* build48_coords = dynarray_create(int);
    dynarray_push_rval(build48_coords, 0);
    dynarray_push_rval(build48_coords, 2);
    build48.BuildUnion.mkbuild.coords = build48_coords;
    build48.BuildUnion.mkbuild.classification = 8;

    BuildUp build50;
    build50.type = MAKE_NODE;
    int* build50_coords = dynarray_create(int);
    dynarray_push_rval(build50_coords, 0);
    dynarray_push_rval(build50_coords, 2);
    build50.BuildUnion.mkbuild.coords = build50_coords;
    build50.BuildUnion.mkbuild.classification = 9;

    BuildUp build51;
    build51.type = MAKE_NODE;
    int* build51_coords = dynarray_create(int);
    dynarray_push_rval(build51_coords, 0);
    dynarray_push_rval(build51_coords, 2);
    build51.BuildUnion.mkbuild.coords = build51_coords;
    build51.BuildUnion.mkbuild.classification = 10;

    BuildUp build_int;
    build_int.type = BOX_NODE;
    build_int.BuildUnion.identifier = 0;

    BuildUp build_float;
    build_float.type = BOX_NODE;
    build_float.BuildUnion.identifier = 1;

    BuildUp build_litstring;
    build_litstring.type = BOX_NODE;
    build_litstring.BuildUnion.identifier = 2;

    BuildUp build_boolean_true;
    build_boolean_true.type = BOX_NODE;
    build_boolean_true.BuildUnion.identifier = 3;

    BuildUp build_boolean_false;
    build_boolean_false.type = BOX_NODE;
    build_boolean_false.BuildUnion.identifier = 4;

    BuildUp build59;
    build59.type = MAKE_NODE;
    int* build59_coords = dynarray_create(int);
    dynarray_push_rval(build59_coords, 0);
    dynarray_push_rval(build59_coords, 2);
    build59.BuildUnion.mkbuild.coords = build59_coords;
    build59.BuildUnion.mkbuild.classification = 101;

    BuildUp build60;
    build60.type = MAKE_NODE;
    int* build60_coords = dynarray_create(int);
    dynarray_push_rval(build60_coords, 0);
    dynarray_push_rval(build60_coords, 2);
    build60.BuildUnion.mkbuild.coords = build60_coords;
    build60.BuildUnion.mkbuild.classification = 102;

    BuildUp build61;
    build61.type = MAKE_NODE;
    int* build61_coords = dynarray_create(int);
    dynarray_push_rval(build61_coords, 0);
    dynarray_push_rval(build61_coords, 2);
    build61.BuildUnion.mkbuild.coords = build61_coords;
    build61.BuildUnion.mkbuild.classification = 103;
    
    BuildUp build_identifier;
    build_identifier.type = BOX_NODE;
    build_identifier.BuildUnion.identifier = 5;

    BuildUp build66;
    build66.type = VALUE;
    build66.BuildUnion.identifier = 0;

    BuildUp build67;
    build67.type = VALUE;
    build67.BuildUnion.identifier = 1;

    BuildUp build68;
    build68.type = VALUE;
    build68.BuildUnion.identifier = 2;

    BuildUp build69;
    build69.type = VALUE;
    build69.BuildUnion.identifier = 3;

    BuildUp build70;
    build70.type = VALUE;
    build70.BuildUnion.identifier = 4;

    //0
    dynarray_push(builds, build_default);
    //1
    dynarray_push(builds, build_default);
    //2
    dynarray_push(builds, build_default_1);
    dynarray_push(builds, build3);
    dynarray_push(builds, build4);
    //5
    dynarray_push(builds, build_default);
    //6
    dynarray_push(builds, build_default);
    //7
    dynarray_push(builds, build_default);
    //8
    dynarray_push(builds, build_default);
    //9
    dynarray_push(builds, build_default);
    //10
    dynarray_push(builds, build_default);
    //11
    dynarray_push(builds, build_default);
    //12
    dynarray_push(builds, build_default);
    //13
    dynarray_push(builds, build_default);
    dynarray_push(builds, build14);
    dynarray_push(builds, build15);
    dynarray_push(builds, build16);
    //17
    dynarray_push(builds, build_default);
    //18
    dynarray_push(builds, build_default);
    dynarray_push(builds, build19);
    dynarray_push(builds, build20);
    dynarray_push(builds, build21);
    dynarray_push(builds, build22);
    dynarray_push(builds, build23);
    dynarray_push(builds, build24);
    //25
    dynarray_push(builds, build_default);
    dynarray_push(builds, build26);
    dynarray_push(builds, build27);
    dynarray_push(builds, build28);
    dynarray_push(builds, build29);
    dynarray_push(builds, build30);
    dynarray_push(builds, build31);
    dynarray_push(builds, build32);
    dynarray_push(builds, build33);
    dynarray_push(builds, build34);
    dynarray_push(builds, build35);
    //36
    dynarray_push(builds, build_default);
    dynarray_push(builds, build37);
    dynarray_push(builds, build38);
    dynarray_push(builds, build39);
    //40
    dynarray_push(builds, build_default);
    dynarray_push(builds, build41);
    dynarray_push(builds, build42);
    dynarray_push(builds, build43);
    dynarray_push(builds, build44);
    dynarray_push(builds, build45);
    //46
    dynarray_push(builds, build_default);
    dynarray_push(builds, build47);
    dynarray_push(builds, build48);
    //47
    dynarray_push(builds, build_default);
    dynarray_push(builds, build50);
    dynarray_push(builds, build51);
    //52
    dynarray_push(builds, build_default);
    //53
    dynarray_push(builds, build_default);
    //54
    dynarray_push(builds, build_int);
    //55
    dynarray_push(builds, build_float);
    //56
    dynarray_push(builds, build_litstring);
    //57
    dynarray_push(builds, build_boolean_true);
    //58
    dynarray_push(builds, build_boolean_false);
    dynarray_push(builds, build59);
    dynarray_push(builds, build60);
    dynarray_push(builds, build61);
    //62
    dynarray_push(builds, build_default);
    //63
    dynarray_push(builds, build_default);
    //64
    dynarray_push(builds, build_default_1);
    //65
    dynarray_push(builds, build_identifier);
    dynarray_push(builds, build66);
    dynarray_push(builds, build67);
    dynarray_push(builds, build68);
    dynarray_push(builds, build69);
    dynarray_push(builds, build70);

    export_token_seq(token, out);

    Grammar G = create_grammar();
    G.builds = builds;
    Subset non_terminals_ss = SS_initialize_empty(symbols_amount);

    int state = 0;
    int head_word_class;
    int* beta_memory = dynarray_create(int);
    bool first_creation = true;
    while(token->category != 0){
        if(state == 0 && token->category == 2){
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