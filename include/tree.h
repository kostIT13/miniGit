#ifndef TREE_H
#define TREE_H

#include "minigit.h"
#include <stdbool.h>

typedef struct TreeNode {
    char *name;            
    char *hash;             
    bool is_file;           
    struct TreeNode *children; 
    int children_count;     
    int ref_count;        
} TreeNode;

TreeNode* tree_create(void);

TreeNode* tree_add_file(TreeNode *old_tree, const char *path, const char *hash);

TreeNode* tree_remove_file(TreeNode *old_tree, const char *path);

const char* tree_get_file_hash(TreeNode *tree, const char *path);

bool tree_file_exists(TreeNode *tree, const char *path);

void tree_print_files(TreeNode *tree, int indent);

void tree_free(TreeNode *tree);

TreeNode* tree_clone_node(TreeNode *node);

#endif