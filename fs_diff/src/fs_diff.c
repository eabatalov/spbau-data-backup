#include <stdio.h>
#include <fs_tree.h>
#include <string.h>
#include <fs_diff.h>

void fs_tree_print_diff(struct fs_tree* old, struct fs_tree* new) {
	if(strcmp(old->head->name, new->head->name) != 0 
			|| old->head->type != new->head->type){
		printf("deleted:\n");
		fs_tree_print(old);
		printf("added:\n");
		fs_tree_print(new);
		return;
	}
	if(old->head->type == INODE_DIR) {
		inode_print_diff((struct dir_inode*)old->head, (struct dir_inode*)new->head);
	}
}

void inode_print_diff(struct dir_inode* old, struct dir_inode* new) {
	/*assume that given inodes are similar*/
	int i = 0;
	int j = 0;
	while(i < old->num_children && j < new->num_children) {
		int compare_res = strcmp(old->children[i]->name, new->children[j]->name);
		if(compare_res == 0 && old->inode.type == new->inode.type) {
			if(old->children[i]->type == INODE_DIR && new->children[i]->type == INODE_DIR) {
				inode_print_diff((struct dir_inode*)old->children[i], (struct dir_inode*)new->children[j]);
			}
			++i;
			++j;
		}
		else if(compare_res < 0) {
			struct fs_tree deleted_branch;
			deleted_branch.head = (struct inode*)old;
			printf("deleted:\n");
			fs_tree_print(&deleted_branch);
			++i;
		}
		else {
			struct fs_tree added_branch;
			added_branch.head = (struct inode*)new;
			printf("added:\n");
			fs_tree_print(&added_branch);
			++j;
		}
	}
}
