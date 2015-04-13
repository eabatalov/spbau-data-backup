#include <stdio.h>
#include <stdlib.h>
#include <file_tree_nodes.h>

int main(int argc, char* argv[]) {
	struct file_tree tree;
	struct dir_inode* tmp_to_get_tree_head;
	
	if(argc < 2) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}

	init_dir_inode(&tmp_to_get_tree_head, argv[1], NULL);
	tree.head = (tmp_to_get_tree_head->inode);
	build_file_tree((struct dir_inode*)&(tree.head));
	print_tree(&(tree.head), 0);
	//free_tree(&tree);
	return 0;
}
