#include <stdio.h>
#include <stdlib.h>
#include <fs_tree.h>

int main(int argc, char* argv[]) {
	struct fs_tree* tree;
	struct dir_inode* tmp_to_get_tree_head;
	
	tmp_to_get_tree_head = (struct dir_inode*)malloc(sizeof(struct dir_inode));
	if(!tmp_to_get_tree_head) {
		perror("Error: failed to allocate memory");
		exit(1);
	}

	if(argc < 2) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}

	tree = fs_tree_collect(argv[1]);
	fs_tree_print(tree);
	fs_tree_destroy(tree);
	return 0;
}
