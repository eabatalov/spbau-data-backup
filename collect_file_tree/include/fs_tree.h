#ifndef _FS_TREE_
#define _FS_TREE_

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

enum inode_type {INODE_REG_FILE, INODE_DEV_FILE, INODE_DIR, INODE_LINK, INODE_MOUNT_POINT, INODE_REG_FILE_DESCR};

struct inode {
	enum inode_type type;
	char* name;
	struct inode* parent;
	struct stat attrs;
	void* user_data;
};

struct regular_file_inode {
	struct inode inode;
};

struct dir_inode {
	struct inode inode;
	size_t num_children;
	struct inode** children;
};

struct fs_tree {
	struct inode* head;
};

typedef int (*fs_tree_inode_visitor)(struct inode* inode, void* data);

struct fs_tree* fs_tree_collect(const char* path);
void fs_tree_destroy(struct fs_tree* tree);
void fs_tree_print(const struct fs_tree *tree);
void fs_tree_bfs(struct fs_tree* fs_tree, fs_tree_inode_visitor visitor, void* data);
void fs_tree_dfs(struct fs_tree* fs_tree, fs_tree_inode_visitor visitor, void* data);

#ifdef __cplusplus
}
#endif

#endif // _FS_TREE_
