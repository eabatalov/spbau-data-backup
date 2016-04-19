#ifndef _FS_TREE_
#define _FS_TREE_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/stat.h>
#include <sys/types.h>


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

struct regular_file_inode* create_regular_file_inode();
struct dir_inode* create_dir_inode();
struct fs_tree* create_fs_tree();
struct stat get_stat_from_data(uid_t uid,  gid_t gid,  time_t atime,  time_t mtime, mode_t mode);

void init_regular_file_inode_from_stat(struct regular_file_inode* dest, const char* name,
                                       struct stat stat, struct inode* parent_dir);
void init_dir_inode_from_stat(struct dir_inode* dest, const char* name, struct stat stat, struct inode* parent_dir, size_t children_count);
void init_fs_tree_from_head(struct fs_tree* dest, struct inode* head);

void free_reg_file_inode(struct regular_file_inode* file_to_delete);
void free_dir_inode(struct dir_inode* dir_to_delete);

#ifdef __cplusplus
}
#endif

#endif // _FS_TREE_
