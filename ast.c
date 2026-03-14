#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "allocator.h"
#include "ast.h"

ASTNode create_node(Arena* arena, int nodetype, ASTNode* children, int children_amount){
    ASTNode new_child;
    new_child.tag = NODE;

    InternalNode* node_storage = (InternalNode*) arena_get(arena, sizeof(InternalNode));
    node_storage->children = children;
    node_storage->amount_children = children_amount;
    node_storage->type = nodetype;

    new_child.storage.node = node_storage;
    return new_child;
}

ASTNode create_label(Arena* arena, char* token_char, int char_length){
    ASTNode new_child;
    new_child.tag = LABEL;

    char* new_label = (char*) arena_get(arena, sizeof(char) * (char_length+1));
    strncpy(new_label, token_char, char_length+1);

    //printf("TOKEN CHAR %s\n", token_char);
    //printf("NEW LABEL %s\n", new_label);
    new_child.storage.label = new_label;
    return new_child;
}

ASTNode create_box(Arena* arena, enum BoxMode boxtype, ASTNode child){
    assert(child.tag == LABEL);
    
    ASTNode new_child;
    new_child.tag = BOX;

    Box* box_storage = (Box*) arena_get(arena, sizeof(Box));
    switch (boxtype) {
        char *endptr;
        case INT_M:
            box_storage->wrapper = INT_WRAPPER;
            box_storage->value.bint = strtol(child.storage.label, &endptr, 10);
            break;
        case FLOAT_M:
            box_storage->wrapper = FLOAT_WRAPPER;
            box_storage->value.bfloat = strtof(child.storage.label, &endptr);
            break;
        case STRING_M:
            // removing the " "
            box_storage->wrapper = STRING_WRAPPER;

            int char_len =  strlen(child.storage.label);
            int new_len = char_len - 2;
            char* str_copy = (char*) arena_get(arena, sizeof(char) * (new_len+1));
            strncpy(str_copy, child.storage.label+1, new_len);
            str_copy[new_len] = '\0';
            box_storage->value.bstring = str_copy;
            break;
        case BOOL_TRUE_M:
            box_storage->wrapper = BOOL_WRAPPER;
            box_storage->value.bint = 1;
            break;
        case BOOL_FALSE_M:
            box_storage->wrapper = BOOL_WRAPPER;
            box_storage->value.bint = 0;
            break;
        case ID_M:
            box_storage->wrapper = ID_WRAPPER;
            box_storage->value.bstring = child.storage.label;
            break;
        default:
            assert(false);
    }

    new_child.storage.box = box_storage;
    return new_child;
}

TreeManager initializeAST(){
    Arena* arena = arena_create(sizeof(ASTNode) * ARENA_CHUNK_SIZE);
    ASTNode root = create_node(arena, ROOT_TYPE, NULL, 0);

    TreeManager manager = {arena, root};
    return manager;
}

void destroyAST(TreeManager tm){
    arena_destroy(tm.arena);
}

void print_ast(ASTNode node, char* prefix, bool is_last) {
    if (prefix[0] != '\0') {
        printf("%s%s", prefix, is_last ? "└── " : "├── ");
    }

    if (node.tag == NODE) {
        InternalNode* internal = node.storage.node;
        printf("[NODE] Type: %d\n", internal->type);

        char next_prefix[512];
        if (prefix[0] == '\0') {
            next_prefix[0] = '\0'; 
        } else {
            snprintf(next_prefix, sizeof(next_prefix), "%s%s", 
                     prefix, is_last ? "    " : "│   ");
        }

        for (int i = 0; i < internal->amount_children; i++) {
            bool last_child = (i == internal->amount_children - 1);
            
            if (prefix[0] == '\0') {
                print_ast(internal->children[i], " ", last_child);
            } else {
                print_ast(internal->children[i], next_prefix, last_child);
            }
        }
    } 
    else if (node.tag == BOX) {
        Box* b = node.storage.box;
        if (b->wrapper == ID_WRAPPER) printf("[ID] %s\n", b->value.bstring);
        else if (b->wrapper == INT_WRAPPER) printf("[INT] %d\n", b->value.bint);
        else if (b->wrapper == FLOAT_WRAPPER) printf("[FLOAT] %.2f\n", b->value.bfloat);
        else if (b->wrapper == STRING_WRAPPER) printf("[STR] %s\n", b->value.bstring);
    }
}

void ast_routine() {
    TreeManager tm = initializeAST();
    Arena* arena = tm.arena;

    printf("--- Building Complex AST: IF-ELSE Statement ---\n\n");

    ASTNode id_x = create_box(arena, ID_M, create_label(arena, "x", 1));
    ASTNode val_10 = create_box(arena, INT_M, create_label(arena, "10", 2));
    
    ASTNode* cond_children = (ASTNode*)arena_get(arena, sizeof(ASTNode) * 2);
    cond_children[0] = id_x;
    cond_children[1] = val_10;
    ASTNode condition = create_node(arena, 101, cond_children, 2);

    ASTNode str_true = create_box(arena, STRING_M, create_label(arena, "\"True\"", 6));
    
    ASTNode* print_args = (ASTNode*)arena_get(arena, sizeof(ASTNode) * 1);
    print_args[0] = str_true;
    ASTNode then_stmt = create_node(arena, 201, print_args, 1);

    ASTNode id_y = create_box(arena, ID_M, create_label(arena, "y", 1));
    ASTNode val_pi = create_box(arena, FLOAT_M, create_label(arena, "3.14", 4));
    
    ASTNode* assign_children = (ASTNode*)arena_get(arena, sizeof(ASTNode) * 2);
    assign_children[0] = id_y;
    assign_children[1] = val_pi;
    ASTNode else_stmt = create_node(arena, 1, assign_children, 2);

    ASTNode* if_children = (ASTNode*)arena_get(arena, sizeof(ASTNode) * 3);
    if_children[0] = condition;
    if_children[1] = then_stmt;
    if_children[2] = else_stmt;
    
    ASTNode root = create_node(arena, 301, if_children, 3);
    tm.root = root;

    print_ast(tm.root, "", true);

    destroyAST(tm);
}