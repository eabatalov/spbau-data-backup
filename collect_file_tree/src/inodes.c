#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <fs_tree.h>
#include <inodes.h>
#include <string.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m" 

void open_dir(char* name, DIR** dest) {
	(*dest) = opendir(name);
	if(!(*dest)) {
		perror("Error: couldn't open the directory");
		exit(1);
	}
}

int get_length_of_name(struct inode* source) {
	if(!source) {
		return 0;
	}
	if(!source->parent) {
		return strlen(source->name) + 1;
	}
	return get_length_of_name(source->parent) + strlen(source->name) + 1;
}

void get_name(char* res, struct inode* source) {
	if(!source) {
		fprintf(stderr, "Warning: trying to get a name of a file which does not exist\n");
		return;
	}
	if(!source->parent) {
		strcpy(res, source->name);
		return;
	}
	get_name(res, source->parent);
	strcat(res, "/");
	strcat(res, source->name);
	return;
}

void init_inode(struct inode* dest, enum inode_type type, const char* name, struct inode* parent) {
	size_t len = strlen(name) + 1;//+1 for terminating null-byte
	dest->type = type;

	dest->name = (char*)malloc(len * sizeof(char));
	if(!dest->name) {
		perror("Error: unable to allocate memory");
		exit(1);
	}
	strcpy(dest->name, name);
	
	dest->parent = parent;
}

void deinit_inode(struct inode* dest) {
	free(dest->name);
}

void init_reg_file_inode(struct regular_file_inode* dest, const char* source_name, struct inode* parent_dir) {
	char* tmp_name;

	init_inode(&(dest->inode), INODE_REG_FILE, source_name, parent_dir);

	tmp_name = (char*)malloc(get_length_of_name(&(dest->inode)) * sizeof(char));
	get_name(tmp_name, &(dest->inode));
	
	
	if(stat(tmp_name, &(dest->inode.attrs)) < 0) {
		perror("Error: failed to get regular file statistics");
		exit(1);
	}
	dest->inode.user_data = NULL;
	
	free(tmp_name);
}

void init_dir_inode(struct dir_inode* dest, const char* dir_name, struct inode* parent_dir) {
	char* tmp_name;

	init_inode(&(dest->inode), INODE_DIR, dir_name, parent_dir);

	tmp_name = (char*)malloc(get_length_of_name(&(dest->inode)) * sizeof(char));
	get_name(tmp_name, &(dest->inode));
	
	
	if(stat(tmp_name, &(dest->inode.attrs)) < 0) {
		perror("Error: failed to get regular file statistics");
		exit(1);
	}
	dest->inode.user_data = NULL;
	dest->num_children = 0;
	dest->children = NULL;
	free(tmp_name);
}

void one_step_bfs_dir(DIR* dir, void (*f)(struct dirent*, void*), void* data) {
	struct dirent* dir_content = readdir(dir);	
	while(dir_content != NULL) {
		f(dir_content, data);
		dir_content = readdir(dir);
	}
}

void implement_number_of_files_in_the_directory(struct dirent* dir_content, void* data) {
	size_t* cnt = (size_t*)data;
	++(*cnt);
}

size_t count_files_in_the_directory(DIR* dir) {
	size_t res = 0;
	one_step_bfs_dir(dir, implement_number_of_files_in_the_directory, (void*)&res);
	return res;
}

void process_dir_child(struct dirent* dir_content, void* data) {
	struct regular_file_inode* tmp_reg_file;
	struct dir_inode* tmp_dir;
	
	struct dir_inode* parent = (struct dir_inode*)data;
	
	switch(dir_content->d_type) {
		case DT_REG:

            tmp_reg_file = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
			if(!tmp_reg_file) {
				perror("Error: failed to allocate memory");
				exit(1);
			}

			init_reg_file_inode(tmp_reg_file, dir_content->d_name, &(parent->inode));			
			parent->children[parent->num_children++] = &(tmp_reg_file->inode);
			break;
		case DT_DIR:
			if(strcmp(dir_content->d_name, ".") != 0 && strcmp(dir_content->d_name, "..") != 0) {
				
				tmp_dir = (struct dir_inode*)malloc(sizeof(struct dir_inode));
				if(!tmp_dir) {
					perror("Error: failed to allocate memory");
					exit(1);
				}
				
				init_dir_inode(tmp_dir, dir_content->d_name, &(parent->inode));
				parent->children[parent->num_children++] = &(tmp_dir->inode);
				build_file_tree(tmp_dir);
			}
			break;
		default:
			break;
	}
}

void process_dir(DIR* dir, struct dir_inode* parent) {
	one_step_bfs_dir(dir, process_dir_child, (void*)parent);
}

void init_parent(DIR* dir, struct dir_inode* parent) {
	size_t size = count_files_in_the_directory(dir);
	rewinddir(dir);
	
	parent->children = (struct inode**)malloc(size * sizeof(struct inode*));
	process_dir(dir, parent);
}

void build_file_tree(struct dir_inode* parent) {
	char* tmp_name;

	DIR* current_dir;//is the directory which parent describes
	
	tmp_name = (char*)malloc(get_length_of_name(&(parent->inode)) * sizeof(char));
	get_name(tmp_name, &(parent->inode));
	open_dir(tmp_name, &current_dir);
	init_parent(current_dir, parent);

	if(closedir(current_dir) < 0) {
		perror("Error: problem while closing the directory\n");
		exit(1);
	}
	free(tmp_name);
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

struct regular_file_inode* create_regular_file_inode() {
    struct regular_file_inode* regular_file_inode = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
    return regular_file_inode;
}

struct dir_inode* create_dir_inode() {
    struct dir_inode* dir_inode = (struct dir_inode*)malloc(sizeof(struct dir_inode));
    return dir_inode;
}

void free_reg_file_inode(struct regular_file_inode* file_to_delete) {
//	free(file_to_delete->inode.name);
    deinit_inode(&(file_to_delete->inode));
    free(file_to_delete);
}

void free_dir_inode(struct dir_inode* dir_to_delete) {
    int i = 0;
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
    deinit_inode(&(dir_to_delete->inode));
    if(dir_to_delete->children) {
        free(dir_to_delete->children);
    }
    free(dir_to_delete);
}

void init_regular_file_inode_from_stat(struct regular_file_inode* dest, const char* name,
                                       struct stat stat, struct inode* parent_dir) {
    init_inode(&(dest->inode), INODE_REG_FILE, name, parent_dir);
    dest->inode.attrs = stat;
}

void init_dir_inode_from_stat(struct dir_inode* dest, const char* name, struct stat stat,struct inode* parent_dir, size_t children_count) {
    init_inode(&(dest->inode), INODE_DIR, name, parent_dir);
    dest->inode.attrs = stat;
    dest->num_children = children_count;
    dest->children = (struct inode**)malloc(children_count * sizeof(struct inode*));
}

void init_fs_tree_from_head(struct fs_tree* dest, struct inode* head) {
    dest->head = head;
}
