#ifndef _FS_DIFF_
#define _FS_DIFF_

void fs_tree_print_diff(struct fs_tree* old, struct fs_tree* new);
void inode_print_diff(struct dir_inode* old, struct dir_inode* new);

#endif
