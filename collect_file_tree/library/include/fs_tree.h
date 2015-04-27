#ifndef _FS_TREE_
#define _FS_TREE_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>


typedef int (*fs_tree_inode_visitor)(struct inode* inode, void* data);

struct fs_tree* fs_tree_collect(const char* path);
void fs_tree_destroy(struct fs_tree* tree);
void fs_tree_print(const struct fs_tree *tree);
void fs_tree_bfs(struct fs_tree* fs_tree, fs_tree_inode_visitor visitor, void* data);
void fs_tree_dfs(struct fs_tree* fs_tree, fs_tree_inode_visitor visitor, void* data);


#endif
