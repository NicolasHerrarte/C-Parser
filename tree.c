#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "dynarray.h"
#include "tree.h"


TreeNode* tree_make_node(int amount_nodes, char* name, TreeNode** nodes){
    TreeNode* new_node = malloc(sizeof(TreeNode));
    new_node->children_amount = amount_nodes;
    // this is new as well
    new_node->name = strdup(name);
    //new_node->name = name;
    new_node->children = malloc(amount_nodes*sizeof(TreeNode*));

    for (int i = 0; i < amount_nodes; i++) {
        new_node->children[i] = nodes[i];
    }

    return new_node;
}

void tree_destroy_node(TreeNode* node){
    free(node->children);
    free(node);
    free(node->name);
}

void destroy_tree(TreeNode* node){

}

void export_node_info(FILE* stream_out, TreeNode* node) {
    if (node == NULL) {
        fprintf(stream_out, "Node is NULL\n");
        return;
    }

    fprintf(stream_out, "[Node] Name: %s | Children: %d\n", 
            node->name, 
            node->children_amount);
}

void print_node_info(TreeNode* node) {
    export_node_info(stdout, node);
}

void export_tree(FILE* stream_out, TreeNode* node, char* prefix, bool is_last, bool is_root) {
    if (!node) return;

    if (is_root) {
        fprintf(stream_out, "%s\n", node->name);
    } else {
        fprintf(stream_out, "%s%s%s\n", prefix, is_last ? "└── " : "├── ", node->name);
    }

    char new_prefix[512];
    if (is_root) {
        new_prefix[0] = '\0';
    } else {
        snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
    }

    for (int i = 0; i < node->children_amount; i++) {
        bool last_child = (i == node->children_amount - 1);

        export_tree(stream_out, node->children[i], new_prefix, last_child, false);
    }
}

void print_tree(TreeNode* node, char* prefix, bool is_last, bool is_root) {
    export_tree(stdout, node, prefix, is_last, is_root);
}