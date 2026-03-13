#include "minigit.h"

typedef struct TreeNode {
    char *name; 
    char *hash;             
    int is_file;           
    struct TreeNode *children; 
    int children_count;
    int ref_count;        
} TreeNode;

typedef struct {
    char *commit_hash;
    char *parent_hash;
    char *message;
    TreeNode *root_tree;    
    time_t timestamp;
} Commit;