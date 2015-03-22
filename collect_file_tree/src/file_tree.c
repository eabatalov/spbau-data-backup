#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <file_tree_nodes.h>
#include <string.h>

void open_dir(char* name, DIR** dest) {
	*dest = opendir(name);
	if(!(*dest)) {
		perror("Error: couldn't open the directory");
		exit(1);
	}
}

struct regular_file_inode* init_reg_file_inode(struct dirent* source, struct inode* parent_dir) {
	size_t len = strlen(source->d_name) + 1;//+1 for terminating null-byte
	size_t parent_dir_len = 0;
	struct regular_file_inode* dest = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
	if(!dest) {
		perror("Error: failed to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		parent_dir_len = strlen(parent_dir->name) + 1;//+1 for '/' after the directory name
	}
	
	dest->inode.type = INODE_REG_FILE;
	
	dest->inode.name = (char*)malloc(len + parent_dir_len);
	if(!dest->inode.name) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		strcpy(dest->inode.name, parent_dir->name);
		strcat(dest->inode.name, "/");//maybe it's too platform-related
	}
	else {
		dest->inode.name[0] = '\0';
	}
	strcat(dest->inode.name, source->d_name);
		
	dest->inode.parent = parent_dir;
	if(stat(dest->inode.name, &dest->inode.attrs) < 0) {
		perror("Error: failed to get regular file statistics");
		exit(1);
	}
	dest->inode.user_data = NULL;
	
	return dest;
}

struct dir_inode* init_dir_inode(const char* dir_name, struct inode* parent_dir) {
	size_t len = strlen(dir_name) + 1;
	size_t parent_dir_len = 0;
	struct dir_inode* dest = (struct dir_inode*)malloc(sizeof(struct dir_inode));
	if(!dest) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		parent_dir_len = strlen(parent_dir->name) + 1;
	}
	
	dest->inode.type = INODE_DIR;
	
	dest->inode.name = (char*)malloc(len + parent_dir_len);
	if(!dest->inode.name) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		strcpy(dest->inode.name, parent_dir->name);
		strcat(dest->inode.name, "/");
	}
	else {
		dest->inode.name[0] = '\0';
	}
	strcat(dest->inode.name, dir_name);

	dest->inode.parent = parent_dir;
	if(stat(dest->inode.name, &dest->inode.attrs) < 0) {
		perror("Error: failed to get regular file statistics");
		exit(1);
	}
	dest->inode.user_data = NULL;
	dest->num_children = 0;
	dest->children = NULL;
	
	return dest;
}

void walk_through_dir(DIR* dir, void (*f)(struct dirent*, void*), void* data) {
	struct dirent* dir_content = readdir(dir);	
	while(dir_content != NULL) {
		f(dir_content, data);
		dir_content = readdir(dir);
	}
}

void count(struct dirent* dir_content, void* data) {
	size_t* cnt = (size_t*)data;
	++(*cnt);
}

size_t count_dir_content(DIR* dir) {
	size_t res = 0;
	walk_through_dir(dir, count, (void*)&res);
	return res;
}

void process_dir_child(struct dirent* dir_content, void* data) {
	struct regular_file_inode* tmp_reg_file = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
	struct dir_inode* tmp_dir = (struct dir_inode*)malloc(sizeof(struct dir_inode));
	struct dir_inode* parent = (struct dir_inode*)data;
	
	switch(dir_content->d_type) {
		case DT_REG:
			tmp_reg_file = init_reg_file_inode(dir_content, &(parent->inode));			
			parent->children[parent->num_children++] = &(tmp_reg_file->inode);
			break;
		case DT_DIR:
			if(strcmp(dir_content->d_name, ".") != 0 && strcmp(dir_content->d_name, "..") != 0) {
				tmp_dir = init_dir_inode(dir_content->d_name, &(parent->inode));
				parent->children[parent->num_children++] = &(tmp_dir->inode);
				build_file_tree(tmp_dir);
			}
			break;
		default:
			break;
	}
}

void process_dir(DIR* dir, struct dir_inode* parent) {
	walk_through_dir(dir, process_dir_child, (void*)parent);
}

void init_parent(DIR* dir, struct dir_inode* parent) {
	size_t size = count_dir_content(dir);
	rewinddir(dir);
	
	parent->children = (struct inode**)malloc(size * sizeof(struct inode*));
	process_dir(dir, parent);
}

void build_file_tree(struct dir_inode* parent) {
	DIR* current_dir;//is the directory which parent describes
	
	open_dir(parent->inode.name, &current_dir);
	init_parent(current_dir, parent);
}

void print_tree(struct inode* node, int space) {
	int i, j;
	for(i = 0; i < space; i++) {
		printf("|	");
	}
	if(node->type == INODE_DIR) {
		printf(ANSI_COLOR_BLUE "%s\n" ANSI_COLOR_RESET, node->name);
		for(j = 0; j < ((struct dir_inode*)node)->num_children; j++) {
			print_tree(((struct dir_inode*)node)->children[j], space + 1);
		}
	}
	else if(node->type == INODE_REG_FILE){
		printf("%s\n", node->name);
	}
	else {
		printf(ANSI_COLOR_RED "???\n" ANSI_COLOR_RESET);
	}
}
