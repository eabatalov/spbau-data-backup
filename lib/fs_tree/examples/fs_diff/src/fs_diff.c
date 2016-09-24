#include <stdio.h>
#include <stdlib.h>
#include <fs_tree.h>
#include <string.h>

int is_in_tree(const char* name, enum inode_type type, struct inode* nodes[], int nodes_count) {
	int cnt = 0;
	while(cnt < nodes_count) {
		if(strcmp(name, nodes[cnt]->name) == 0 && type == nodes[cnt]->type) {
			return cnt;
		}
		++cnt;
	}
	return -1;
}//efficiency!!! %)

void inode_print_diff(struct dir_inode* old, struct dir_inode* new) {
	/*assume that given inodes are similar*/
	int i = 0;

	printf("Entering directory '%s'\n", old->inode.name);
	for(i = 0; i < old->num_children; i++) {
		int it = is_in_tree(old->children[i]->name, 
			      old->children[i]->type,
			      new->children,
			      new->num_children);
		if(it < 0) {
			struct fs_tree deleted_node;
			deleted_node.head = old->children[i];
			printf("-----deleted-----\n");
			fs_tree_print(&deleted_node);
			printf("----/deleted-----\n");
		}
	}
	for(i = 0; i < new->num_children; i++) {
		int it = is_in_tree(new->children[i]->name, 
			      new->children[i]->type,
			      old->children,
			      old->num_children);
			if(it < 0) {
			struct fs_tree deleted_node;
			deleted_node.head = new->children[i];
			printf("-----added-----\n");
			fs_tree_print(&deleted_node);
			printf("----/added-----\n");
		}
	}
	for(i = 0; i < old->num_children; i++) {
		int it = is_in_tree(old->children[i]->name, 
			      old->children[i]->type,
			      new->children,
			      new->num_children);
		if(it >= 0) {
			if(old->children[i]->type == INODE_DIR) {
				inode_print_diff((struct dir_inode*)old->children[i], 
						 (struct dir_inode*)new->children[it]);
			}
		}
	}
	printf("Leaving directory  '%s'\n", old->inode.name);
}

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
		inode_print_diff((struct dir_inode*)old->head, 
				 (struct dir_inode*)new->head);
	}
}


int main(int argc, char* argv[]) {
	struct fs_tree* tree1;
	struct fs_tree* tree2;

	if(argc < 3) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}

	tree1 = fs_tree_collect(argv[1]);
	tree2 = fs_tree_collect(argv[2]);
	if(strlen(tree1->head->name) !=
			strlen(tree2->head->name)) {
		fprintf(stderr, "Err:lengths of the given directories should be equal :3");
		exit(1);
	}

	strcpy(tree1->head->name, tree2->head->name);
	fs_tree_print_diff(tree1, tree2);
	printf("\nBoth trees:\n");
	fs_tree_print(tree1);
	printf("\n");
	fs_tree_print(tree2);

	fs_tree_destroy(tree1);
	fs_tree_destroy(tree2);
	
	return 0;
}
