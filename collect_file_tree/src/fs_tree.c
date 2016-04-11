#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inodes.h>
#include <fs_tree.h>


struct fs_tree* fs_tree_collect(const char* path) {
	struct fs_tree* tree = (struct fs_tree*)malloc(sizeof(struct fs_tree));
	struct regular_file_inode* reg_file_tmp;
	struct dir_inode* dir_tmp; //tmp_to_get_tree_head = (struct dir_inode*)malloc(sizeof(struct dir_inode));
	struct stat buf = {};

	if(stat(path, &buf) < 0) {
		perror("Error: failed to get file statistics\n");
		exit(1);
	}

	if(S_ISDIR(buf.st_mode)) {
		dir_tmp = (struct dir_inode*)malloc(sizeof(struct dir_inode));
 		if(!dir_tmp) {
			perror("Error: failed to allocate memory");
			exit(1);
		}

		init_dir_inode(dir_tmp, path, NULL);
		tree->head = &(dir_tmp->inode);
		build_file_tree((struct dir_inode*)(tree->head));
	}
	else if(S_ISREG(buf.st_mode)) {
		reg_file_tmp = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
		if(!reg_file_tmp) {
			perror("Error: failed to allocate memory\n");
			exit(1);
		}

		init_reg_file_inode(reg_file_tmp, path, NULL);
		tree->head = &(reg_file_tmp->inode);
	}

	return tree;
}

void fs_tree_destroy(struct fs_tree* tree) {
	if(tree->head->type == INODE_DIR) {
		free_tree(tree);
	}
	else if(tree->head->type == INODE_REG_FILE) {
		free((struct regular_file_inode*)tree->head);
	}
	free(tree);
}

void fs_tree_print(const struct fs_tree* tree) {
	print_tree(tree->head, 0);
}

struct regular_file_inode* create_regular_file_inode() {
    struct regular_file_inode* regular_file_inode = (struct regular_file_inode*)malloc(sizeof(struct regular_file_inode));
    return regular_file_inode;
}

struct dir_inode* create_dir_inode() {
    struct dir_inode* dir_inode = (struct dir_inode*)malloc(sizeof(struct dir_inode));
    return dir_inode;
}

struct fs_tree* create_fs_tree() {
    struct fs_tree* tree = (struct fs_tree*)malloc(sizeof(struct fs_tree));
    return tree;
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
//	free(dir_to_delete->inode.name);
    deinit_inode(&(dir_to_delete->inode));
    if(dir_to_delete->children) {
        free(dir_to_delete->children);
    }
    free(dir_to_delete);
}

void free_tree(struct fs_tree* tree) {
    free_dir_inode((struct dir_inode*)(tree->head));
}

void init_inode_from_stat(struct inode* dest, enum inode_type type, const char* name, struct stat stat,
                          struct inode* parent_dir) {
    size_t len = strlen(name) + 1;
    dest->type = type;
    dest->attrs = stat;

    dest->name = (char*)malloc(len * sizeof(char));
    if(!dest->name) {
        perror("Error: unable to allocate memory");
        exit(1);
    }
    strcpy(dest->name, name);
    dest->user_data = NULL;
    dest->parent = parent_dir;
}

void init_regular_file_inode_from_stat(struct regular_file_inode* dest, const char* name,
                                       struct stat stat, struct inode* parent_dir) {
    init_inode_from_stat(&(dest->inode), INODE_REG_FILE, name, stat, parent_dir);
}

void init_dir_inode_from_stat(struct dir_inode* dest, const char* name, struct stat stat,struct inode* parent_dir, size_t children_count) {
    init_inode_from_stat(&(dest->inode), INODE_DIR, name, stat, parent_dir);
    dest->num_children = children_count;
    dest->children = (struct inode**)malloc(children_count * sizeof(struct inode*));
}

void init_fs_tree_from_stat(struct fs_tree* dest, struct inode* head) {
    dest->head = head;
}

struct stat get_stat_from_data(uid_t uid,  gid_t gid,  time_t atime,  time_t mtime, mode_t mode) {
    struct stat stat;
    stat.st_uid = uid;
    stat.st_gid = gid;
    stat.st_mtime = mtime;
    stat.st_atime = atime;
    stat.st_mode = mode;
    return stat;
}
