#include <inodes.h>
#include <fs_tree.h>

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
