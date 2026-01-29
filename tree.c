#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "dynarray.h"
#include "tree.h"


TreeNode* tree_make_node(int amount_nodes, char* name, TreeNode** nodes){
    TreeNode* new_node = malloc(sizeof(TreeNode));
    new_node->children_amount = amount_nodes;
    new_node->name = name;
    new_node->children = malloc(amount_nodes*sizeof(TreeNode*));

    for (int i = 0; i < amount_nodes; i++) {
        new_node->children[i] = nodes[i];
    }

    return new_node;
}

void print_node_info(TreeNode* node) {
    if (node == NULL) {
        printf("Node is NULL\n");
        return;
    }

    printf("[Node] Name: %s | Children: %d\n", 
            node->name, 
            node->children_amount);
}

void print_tree(TreeNode* node, char* prefix, bool is_last, bool is_root) {
    if (!node) return;

    if (is_root) {
        // No branch characters for the very first node
        printf("%s\n", node->name);
    } else {
        // Standard branch for everyone else
        printf("%s%s%s\n", prefix, is_last ? "└── " : "├── ", node->name);
        //printf("%s%s%s\n", prefix, is_last ? "└── " : "├── ", node->name);
    }

    // Update prefix only if we aren't at the root
    char new_prefix[512];
    if (is_root) {
        new_prefix[0] = '\0';
    } else {
        snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
    }

    for (int i = 0; i < node->children_amount; i++) {
        bool last_child = (i == node->children_amount - 1);
        // All subsequent calls are NOT the root
        print_tree(node->children[i], new_prefix, last_child, false);
    }
}