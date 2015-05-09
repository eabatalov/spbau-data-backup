#include <inodes.h>
#include <fs_tree.h>

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
