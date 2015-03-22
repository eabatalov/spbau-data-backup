#include <stdio.h>
#include <stdlib.h>
#include <file_tree_nodes.h>

int main(int argc, char* argv[]) {
	struct file_tree tree;

	if(argc < 2) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}

	tree.head = &((init_dir_inode(argv[1], NULL))->inode);
	build_file_tree((struct dir_inode*)tree.head);
	print_tree(tree.head, 0);
	return 0;
}
