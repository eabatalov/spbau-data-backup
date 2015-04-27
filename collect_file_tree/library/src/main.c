#include <stdio.h>
#include <stdlib.h>
#include <file_tree_nodes.h>
#include <fs_tree.h>

int bfs_visitor(struct inode* inode, void* data) {
	int* r = (int*)data;
	printf(ANSI_COLOR_YELLOW "%d\n" ANSI_COLOR_RESET, *r);
	return ++(*r);
}

int dfs_visitor(struct inode* inode, void* data) {
	int* r = (int*)data;
	printf(ANSI_COLOR_GREEN "%s\n" ANSI_COLOR_RESET, inode->name);
	//fprintf(stderr, "node: %s\n", inode->name);
	++(*r);
	return (*r);
}

int main(int argc, char* argv[]) {
	struct fs_tree* tree;
	//int a = 0;

	if(argc < 2) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}

	tree = fs_tree_collect(argv[1]);
	fs_tree_print(tree);
	//fs_tree_dfs(tree, dfs_visitor, &a);
	//a = 0;
	//fs_tree_bfs(tree, bfs_visitor, &a);
	fs_tree_destroy(tree);
	return 0;
}
