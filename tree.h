#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

typedef struct TreeNode{
    char* name;
    int children_amount;
    struct TreeNode** children;
} TreeNode;

TreeNode* tree_make_node(int amount_nodes, char* name, TreeNode** nodes);
void print_tree(TreeNode* node, char* prefix, bool is_last, bool is_root);;
void print_node_info(TreeNode* node);