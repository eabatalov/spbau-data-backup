#include <stdio.h>
#include <stdlib.h>
#include <inodes.h>
#include <fs_tree.h>


struct fs_tree* fs_tree_collect(const char* path) {
	struct fs_tree* tree = (struct fs_tree*)malloc(sizeof(struct fs_tree));
	struct regular_file_inode* reg_file_tmp;
	struct dir_inode* dir_tmp; //tmp_to_get_tree_head = (struct dir_inode*)malloc(sizeof(struct dir_inode));
	struct stat* buf = (struct stat*)malloc(sizeof(struct stat));

	if(!buf) {
		perror("Error: failed to allocate memory\n");
		exit(1);
	}
	if(stat(path, buf) < 0) {
		perror("Error: failed to get file statistics\n");
		exit(1);
	}

	if(S_ISDIR(buf->st_mode)) {
		dir_tmp = (struct dir_inode*)malloc(sizeof(struct dir_inode));
 		if(!dir_tmp) {
			perror("Error: failed to allocate memory");
			exit(1);
		}

		init_dir_inode(dir_tmp, path, NULL);
		tree->head = &(dir_tmp->inode);
		build_file_tree((struct dir_inode*)(tree->head));
	}
	else if(S_ISREG(buf->st_mode)) {
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
