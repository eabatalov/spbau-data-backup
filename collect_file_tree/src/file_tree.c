#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <file_tree_nodes.h>
#include <string.h>

void open_dir(char* name, DIR** dest) {
	fprintf(stderr, "%s\n", __func__);

	(*dest) = opendir(name);
	if(!(*dest)) {
		perror("Error: couldn't open the directory");
		exit(1);
	}
}

void init_reg_file_inode(struct regular_file_inode** dest, struct dirent* source, struct inode* parent_dir) {
	fprintf(stderr, "%s\n", __func__);

	size_t len = strlen(source->d_name) + 1;//+1 for terminating null-byte
	size_t parent_dir_len = 0;
	(*dest) = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
	if(!(*dest)) {
		perror("Error: failed to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		parent_dir_len = strlen(parent_dir->name) + 1;//+1 for '/' after the directory name
	}
	
	(*dest)->inode.type = INODE_REG_FILE;
	
	(*dest)->inode.name = (char*)malloc(len + parent_dir_len);
	if(!(*dest)->inode.name) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		strcpy((*dest)->inode.name, parent_dir->name);
		strcat((*dest)->inode.name, "/");//maybe too platform-related
	}
	else {
		(*dest)->inode.name[0] = '\0';	
	}
	strcat((*dest)->inode.name, source->d_name);
		
	(*dest)->inode.parent = parent_dir;
	if(stat((*dest)->inode.name, &(*dest)->inode.attrs) < 0) {
		perror("Error: failed to get regular file statistics");
		exit(1);
	}
	(*dest)->inode.user_data = NULL;
}

void init_dir_inode(struct dir_inode** dest, const char* dir_name, struct inode* parent_dir) {
	fprintf(stderr, "%s\n", __func__);

	size_t len = strlen(dir_name) + 1;
	size_t parent_dir_len = 0;
	(*dest) = (struct dir_inode*)malloc(sizeof(struct dir_inode));
	if(!(*dest)) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		parent_dir_len = strlen(parent_dir->name) + 1;
	}
	
	(*dest)->inode.type = INODE_DIR;
	
	(*dest)->inode.name = (char*)malloc(len + parent_dir_len);
	if(!(*dest)->inode.name) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	if(parent_dir) {
		strcpy((*dest)->inode.name, parent_dir->name);
		strcat((*dest)->inode.name, "/");
	}
	else {
		(*dest)->inode.name[0] = '\0';
	}
	strcat((*dest)->inode.name, dir_name);

	(*dest)->inode.parent = parent_dir;
	if(stat((*dest)->inode.name, &(*dest)->inode.attrs) < 0) {
		perror("Error: failed to get regular file statistics");
		exit(1);
	}
	(*dest)->inode.user_data = NULL;
	(*dest)->num_children = 0;
	(*dest)->children = NULL;
}

void walk_through_dir(DIR* dir, void (*f)(struct dirent*, void*), void* data) {
	fprintf(stderr, "%s\n", __func__);

	struct dirent* dir_content = readdir(dir);	
	while(dir_content != NULL) {
		f(dir_content, data);
		dir_content = readdir(dir);
	}
}

void implement_number_of_files_in_the_directory(struct dirent* dir_content, void* data) {
	fprintf(stderr, "%s\n", __func__);

	size_t* cnt = (size_t*)data;
	++(*cnt);
}

size_t count_files_in_the_directory(DIR* dir) {
	fprintf(stderr, "%s\n", __func__);

	size_t res = 0;
	walk_through_dir(dir, implement_number_of_files_in_the_directory, (void*)&res);
	return res;
}

void process_dir_child(struct dirent* dir_content, void* data) {
	fprintf(stderr, "%s\n", __func__);

	struct regular_file_inode* tmp_reg_file;
	struct dir_inode* tmp_dir;
	
	struct dir_inode* parent = (struct dir_inode*)data;
	
	switch(dir_content->d_type) {
		case DT_REG:
			init_reg_file_inode(&tmp_reg_file, dir_content, &(parent->inode));			
			parent->children[parent->num_children++] = &(tmp_reg_file->inode);
			break;
		case DT_DIR:
			if(strcmp(dir_content->d_name, ".") != 0 && strcmp(dir_content->d_name, "..") != 0) {
				init_dir_inode(&tmp_dir, dir_content->d_name, &(parent->inode));
				parent->children[parent->num_children++] = &(tmp_dir->inode);
				build_file_tree(tmp_dir);
			}
			break;
		default:
			break;
	}
}

void process_dir(DIR* dir, struct dir_inode* parent) {
	fprintf(stderr, "%s\n", __func__);

	walk_through_dir(dir, process_dir_child, (void*)parent);
}

void init_parent(DIR* dir, struct dir_inode* parent) {
	fprintf(stderr, "%s\n", __func__);

	size_t size = count_files_in_the_directory(dir);
	rewinddir(dir);
	
	parent->children = (struct inode**)malloc(size * sizeof(struct inode*));
	process_dir(dir, parent);
}

void build_file_tree(struct dir_inode* parent) {
	fprintf(stderr, "%s\n", __func__);

	DIR* current_dir;//is the directory which parent describes
	
	open_dir(parent->inode.name, &current_dir);
	init_parent(current_dir, parent);
}

void print_tree(struct inode* node, int space) {
	fprintf(stderr, "%s\n", __func__);

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

void free_reg_file_inode(struct regular_file_inode* file_to_delete) {
	fprintf(stderr, "%s\n", __func__);

	fprintf(stderr, "file: %s %p\n", file_to_delete->inode.name, file_to_delete);
	free(file_to_delete);
}

void free_dir_inode(struct dir_inode* dir_to_delete) {
	fprintf(stderr, "%s\n", __func__);

	int i = 0;
	fprintf(stderr, "dir: %s %p\n", dir_to_delete->inode.name, dir_to_delete);
	for(; i < dir_to_delete->num_children; i++) {
		if(dir_to_delete->children[i]->type == INODE_DIR) {
			free_dir_inode((struct dir_inode*)(dir_to_delete->children[i]));
		}
		else if(dir_to_delete->children[i]->type == INODE_REG_FILE){
			free_reg_file_inode((struct regular_file_inode*)(dir_to_delete->children[i]));
		}
		else {
			fprintf(stderr, "Something unidentified...\n");
		}
	}
	free(dir_to_delete);
}

void free_tree(struct file_tree* tree) {
	fprintf(stderr, "%s\n", __func__);

	free_dir_inode((struct dir_inode*)&(tree->head));
	fprintf(stderr, "freeing is completed\n");
}

/*
todo:
	1. to fix SegFault
	2. to fix names
*/
