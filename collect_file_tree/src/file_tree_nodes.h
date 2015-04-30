#ifndef _FILE_TREE_NODES_
#define _FILE_TREE_NODES_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>


struct inode;
struct regular_file_inode;
struct dir_inode;
struct fs_tree;


void open_dir(char* name, DIR** dest);
int get_length_of_name(struct inode* source);
void get_name(char* res, struct inode* source);
void init_reg_file_inode(struct regular_file_inode* dest, struct dirent* source, struct inode* parent_dir);
void init_dir_inode(struct dir_inode* dest, const char* dir_name, struct inode* parent_dir);
void one_step_bfs_dir(DIR* dir, void (*f)(struct dirent*, void*), void* data);
void implement_number_of_files_in_the_directory(struct dirent* dir_content, void* data);
size_t count_files_in_the_directory(DIR* dir);
void process_dir_child(struct dirent* dir_content, void* data);
void process_dir(DIR* dir, struct dir_inode* parent);
void init_parent(DIR* , struct dir_inode* parent);
void build_file_tree(struct dir_inode* parent);
void print_tree(struct inode* node, int space);
void free_reg_file_inode(struct regular_file_inode* file_to_delete);
void free_dir_inode(struct dir_inode* dir_to_delete);
void free_tree(struct fs_tree* tree);



#endif
