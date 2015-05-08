#include <stdio.h>
#include <stdlib.h>
#include <file_tree_nodes.h>
#include <fs_tree.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

void dir_dfs(struct dir_inode* dir, fs_tree_inode_visitor visit, void* data) {
	int i = 0;
	int running = 1;
	for(; i < dir->num_children && running; i++) {
		running = visit(dir->children[i], data);
		if(dir->children[i]->type == INODE_DIR) {
			if(running) {
				dir_dfs((struct dir_inode*)dir->children[i], visit, data);
			}
		}
	}
}

void fs_tree_dfs(struct fs_tree* fs_tree, fs_tree_inode_visitor visit, void* data) {
	if(fs_tree->head->type == INODE_DIR) {
		dir_dfs((struct dir_inode*)fs_tree->head, visit, data);
	}
	else if(fs_tree->head->type == INODE_REG_FILE) {
		visit(fs_tree->head, data);
	}
}

int implement_number_of_files (struct inode* inode, void* data) {
	size_t* cnt = (size_t*)data;
	++(*cnt);
	return 1;
}

size_t get_tree_size(struct fs_tree* fs_tree) {
	size_t res = 0;
	fs_tree_dfs(fs_tree, implement_number_of_files, &res);
	return res;
}

void dir_bfs(fs_tree_inode_visitor visit, void* data, struct inode* queue[], 
				int* queue_start, int* queue_end, int* running) {
	
	int i;
	struct inode* node = queue[(*queue_start)++];
	*running = visit(node, data);
	if(node->type == INODE_DIR && *running) {
		struct dir_inode* dir = (struct dir_inode*)node;
		for(i = 0; i < dir->num_children; i++) {
			queue[(*queue_end)++] = dir->children[i];
		}
	}
}

void fs_tree_bfs(struct fs_tree* fs_tree, fs_tree_inode_visitor visit, void* data) {
	if(fs_tree->head->type == INODE_DIR) {
		size_t queue_size = get_tree_size(fs_tree) + 1;
		int queue_start = 0;
		int queue_end = 0;
		int running = 1;
		struct inode** queue = (struct inode**)malloc(queue_size * sizeof(struct inode*));
		
		queue[queue_end++] = fs_tree->head;
		while(queue_start < queue_end && running) {
			dir_bfs(visit, data, queue, &queue_start, &queue_end, &running);
		}
	
		free(queue);
	}
	else if (fs_tree->head->type == INODE_REG_FILE) {
		visit(fs_tree->head, data);
	}
}
