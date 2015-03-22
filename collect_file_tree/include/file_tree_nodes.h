#ifndef _FILE_TREE_NODES_
#define _FILE_TREE_NODES_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m" 

enum inode_type {INODE_REG_FILE, INODE_DEV_FILE, INODE_DIR, INODE_LINK, INODE_MOUNT_POINT, INODE_REG_FILE_DESCR};

struct inode {
	enum inode_type type;
	char* name;
	struct inode* parent;
	struct stat attrs;
	void* user_data;
};

struct regular_file_inode {
	struct inode inode;
};

struct dir_inode {
	struct inode inode;
	size_t num_children;
	struct inode** children;
};

struct file_tree {
	struct inode* head;
};

void open_dir(char* , DIR** );
struct regular_file_inode* init_reg_file_inode(struct dirent* , struct inode* );
struct dir_inode* init_dir_inode(const char* , struct inode* );
void walk_through_dir(DIR* , void (*f)(struct dirent*, void*), void* );
void count(struct dirent* , void* );
size_t count_dir_content(DIR* );
void process_dir_child(struct dirent* , void* );
void process_dir(DIR* , struct dir_inode* );
void init_parent(DIR* , struct dir_inode* );
void build_file_tree(struct dir_inode*);
void print_tree(struct inode* , int );

#endif
