#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <file_tree_nodes.h>

void open_dir(char* name, DIR** dest) {
	*dest = opendir(name);
	if(!(*dest)) {
		perror("Error: couldn't open the directory\n");
		exit(1);
	}
}

void init_reg_file_inode(struct regular_file_inode *dest, struct dirent* source, struct inode* parent_dir) {
	dest = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
	dest->inode.type = INODE_REG_FILE;
	strcpy(dest->inode.name, source->d_name);
	dest->inode.parent(parent_dir);
	if(stat(dest->inode.name, dest->inode.attrs) < 0) {
		perror("Error: failed to get regular file statistics\n");
		exit(1);
	}
	dest->inode.user_data = NULL;
}

void init_dir_inode(struct dir_inode* dest, const char* dir_name, struct inode* parent_dir) {
	dest = (struct dir_inode*)malloc(sizeof(dir_inode));
	dest->inode.type = INODE_DIR;
	strcpy(dest->inode.name, dir_name);
	dest->inode.parent(parent_dir);
	if(stat(dest->inode.name, dest->inode.attrs) < 0) {
		perror("Error: failed to get regular file statistics\n");
		exit(1);
	}
	dest->inode.user_data = NULL;
	dest->num_children = 0;
	dest->children = NULL;
}

void walk_through_dir(DIR* dir, void (*f)(struct dirent*, void*), void* data) {
	struct dirent* dir_content;
	while(dir_content = readdir(dir)) {
		f(dir_content, data);
	}
}

void count(struct dirent* dir_content, void* data) {
	size_t* cnt = (int*)data;
	++(*cnt);
}

size_t count_dir_content(DIR* dir) {
	size_t res = 0;
	walk_through_dir(dir, count, (void*)&res);
	return res;
}

void process_dir_child(struct dirent* dir_content, void* data) {
	struct regular_file_inode* tmp_reg_file;
	struct dir_inode* tmp_dir;
	struct dir_inode* parent = (struct dir_inode*)data;
	
	switch(dir_content->d_type) {
		case DT_REG:
			init_reg_file_inode(tmp_reg_file, dir_content, &(parent->inode));
			parent->children[parent->num_children++] = &(tmp_reg_file->inode);
			break;
		case DT_DIR:
			init_dir_inode(tmp_dir, dir_content->d_name, &(parent->inode));
			parent->children[parent->num_children++] = &(tmp_dir->inode);
			build_file_tree(tmp_dir);
			break;
		default:
			fprintf(stderr, "Unknown type of a directory content\n");
			break;
	}
}

void process_dir(DIR* dir, struct dir_inode* parent) {
	walk_through_dir(dir, (void*)parent);
}

void init_parent(DIR* dir, struct dir_inode* parent) {
	size_t size = count_dir_content(current_dir);
	parent->children = (inode**)malloc(size * sizeof(inode*));
	process_dir(dir, parent);
}

void build_file_tree(struct dir_inode* parent) {
	DIR* current_dir;
	
	open_dir(parent->inode.name, &current_dir);
	init_parent(dir, parent);
}

void print_tree(struct inode* node, int space) {
	int i, j;
	for(i = 0; i < space; i++) {
		printf("|		");
	}
	if(node->type == INODE_DIR) {
		print(ANSI_COLOR_BLUE "%s\n" ANSI_COLOR_RESET, node->name);
		for(j = 0; j < (dir_inode*)node->num_children; j++) {
			print_tree((dir_inode*)node->children[j], space + 1);
		}
	}
	else if(node->type == INODE_REG_FILE){
		printf("%s\n", node->name);
	}
	else {
		printf(ANSI_COLOR_RED "???\n" ANSI_COLOR_RESET);
	}
}
