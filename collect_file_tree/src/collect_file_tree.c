#include <stdio.h>
#include <stdlib.h>
#include <file_tree_nodes.h>

int main(int argc, char* argv[]) {
	struct file_tree tree;
	struct dir_inode tree_head;
	
	if(argc < 2) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}
	init_dir_inode(&tree_head, argv[1], NULL);
	tree.head = &(tree_head.inode);
	build_file_tree(&tree_head);
	print_tree(tree.head, 0);
	
	return 0;
}
