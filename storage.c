#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "blocks.h"
#include "directory.h"
#include "inode.h"
#include "storage.h"

// checks if a file is acessible
int storage_access(const char* path) {
//checks if a file exists using the path input
int inum = tree_lookup(path);
  if (inum < 0) {
    return -ENOENT;
  }
  // gets the node given our inum
  inode_t* node = get_inode(inum);
  return 0;
}

// initialize the blocks and directory
void storage_init(const char *path) {
  // initialize a directory and a block once the
  // function is run
  blocks_init(path);
  directory_init();
}

// store the statistic into storage stat
int storage_stat(const char *path, struct stat *st) {
  int inum = tree_lookup(path);
  
  //checks that the file exists
  if (inum < 0) {
    return -ENOENT;
  }
  // sets the statistics into the storage stat	  
  inode_t* nd = get_inode(inum);
  memset(st, 0, sizeof(struct stat));
  st->st_uid = getuid();
  st->st_mode = nd->mode;
  st->st_size = nd->size;
       
  return 0;
}

// read from the file and stored in the buf
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  // iterators for copying the memory using buffer and data
  int tempsize = size;
  int tempoffset = offset;
  int i = 0;
  int inum = tree_lookup(path);
  // gets the node given the inum we have from our path
  inode_t *node = get_inode(inum);
  // loop through and read the file using the iterators and memory
  while (tempsize > 0) {
    int bnum = inode_get_bnum(node, tempoffset);
    // this will be the data we will read
    char* data = (char*) blocks_get_block(bnum);
    data += tempoffset % BLOCK_SIZE;
    int min = BLOCK_SIZE - (tempoffset % BLOCK_SIZE);
    if (tempsize < min) {
      min = tempsize;
    }
    // copying memory using the iterator
    memcpy(buf + i, data, min);
    i += min;
    tempoffset += min;
    tempsize -= min;
  }
  return size;
}

// write the buffer to the file
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
  // iterators used to write into the file using memcpy data and buf
  int tempsize = size;
  int tempoffset = offset;
  int i = 0;
  int inum = tree_lookup(path);
  // gets the node given the inum from our path
  inode_t* node = get_inode(inum);
  // the new size of the file/node
  int new_size = size + offset;
  if (node->size < new_size) {
    storage_truncate(path, new_size);
  }
  // loop to write the data to the file using our iterators
  while (tempsize > 0) {
    int bnum = inode_get_bnum(node, tempoffset);
    // this is the data we will be writing into the new file
    char* data = (char*) blocks_get_block(bnum);
    data += tempoffset % BLOCK_SIZE;
    int min = BLOCK_SIZE - (tempoffset % BLOCK_SIZE);
    if (tempsize < min) {
      min = tempsize;
    }
    // this is where data is being written
    memcpy(data, buf + i, min);
    i += min;
    tempoffset += min;
    tempsize -= min;	
  }
  
  return size;
}

// updates the size
int storage_truncate(const char *path, off_t size) {
   int inum = tree_lookup(path);
    if(inum < 0) {
        return -ENOENT;           
    }
    inode_t* node = get_inode(inum);
    if(size >= node->size) {
      grow_inode(node, size);
    } else {
      shrink_inode(node, size);
    }
  return 0;
}

// add a directory to the path
// we understand this is 50+ lines, please be lenient this is the only one :(
int storage_mknod(const char *path, int mode) {
  if (tree_lookup(path) > -1) {
    return -EEXIST;
  }
  // we get the path here
  char* full_path = (char*) malloc(strlen(path) + 10);
  memset(full_path, 0, strlen(path) + 10);
  full_path[0] = '/';
  slist_t* items = s_explode(path + 1, '/');
  for (slist_t* other = items; other != NULL; other = other->next) {
    int dir_value = tree_lookup(full_path);
    int sub_value = directory_lookup(get_inode(tree_lookup(full_path)), other->data);
    if (other->next == NULL && sub_value < 0) {
      int inum = alloc_inode();
      inode_t* node = get_inode(inum); 
      node->mode = mode;
      if (node->mode == 040755) {
        char* selfname = ".";
        char* parentname = "..";
        directory_put(node, parentname, tree_lookup(full_path));
        directory_put(node, selfname, inum);
      }

      int rv = directory_put(get_inode(tree_lookup(full_path)), other->data, inum);
      if (rv != 0) {
        free(full_path);
        return -1;
      }
      node->refs = 1;
      free(full_path);
      return 0;
    } else if (sub_value > -1 && get_inode(sub_value) -> mode == 040755) {
      if (strcmp(full_path, "/") == 0) {
        strcpy(full_path + strlen(full_path), other->data);
        continue;
      } else {
        strcpy(full_path + strlen(full_path), "/");
        strcpy(full_path + strlen(full_path), other->data);
        continue;
      }
    } else {  
      int inum = alloc_inode();
      inode_t *node = get_inode(inum); 
      node->mode = 040755;
      char *selfname = ".";
      char *parentname = "..";
      directory_put(node, parentname, dir_value);
      directory_put(node, selfname, inum);
      directory_put(get_inode(dir_value), other->data, inum);
      node->refs = 1;
      if (strcmp(full_path, "/") == 0) {
        strcat(full_path, other->data);
        continue;
      } else {
        strcat(full_path, "/");
        strcat(full_path, other->data);
        continue;
      }
    }
  }
}
  
    


// given the full path, parses the parent directory of it
void parse_parent(const char *full_path, char *dir) {
  int len = 0;
  strcpy(dir, full_path);
  for (int i = strlen(full_path) - 1; full_path[i] != '/'; i--) {
    len++;
  }
  if (len == strlen(full_path) - 1) {
    dir[1] = '\0';
  } else {
    dir[strlen(full_path) - len - 1] = '\0';
  }
}

// given the full path, parses the child of it
char *parse_child(const char *full_path, char *sub) {
  strcpy(sub, full_path);
  int len = 0;
  for (int i = strlen(full_path) - 1; full_path[i] != '/'; i--) {
    len++;
  }
  sub += strlen(full_path) - len;
  return sub;
}

// unlinks a file (deletion) 
int storage_unlink(const char *path) {
  char* dir = (char*) alloca(strlen(path) + 1);
  char* sub = (char*) alloca(strlen(path) + 1);
  parse_parent(path, dir);
  sub = parse_child(path, sub);
  int dirnum = tree_lookup(dir);
  int subnum = tree_lookup(path);
  inode_t* dirnode = get_inode(dirnum);
  inode_t* subnode = get_inode(subnum);
  int rv = directory_delete(dirnode, sub);
  return rv;
}

// links things together 
int storage_link(const char *from, const char *to) {
  int toNum = tree_lookup(to);
  if (toNum < 0) {
    return toNum;
  }

  char* fromParent = (char*) alloca(strlen(from) + 1);
  char* fromChild = (char*) alloca(strlen(from) + 1);
  parse_parent(from, fromParent);
  fromChild = parse_child(from, fromChild);
  int fromParentNum = tree_lookup(fromParent); 
  inode_t* fromParentNode = get_inode(fromParentNum);
  int rv = directory_put(fromParentNode, fromChild, toNum);

  return 0;
}

// method used to remname things
int storage_rename(const char *from, const char *to) {
  storage_link(to, from);
  storage_unlink(from);
  return 0;
}

// todo
int storage_set_time(const char *path, const struct timespec ts[2]) {
  return 0;
}

// returns the storage list
slist_t *storage_list(const char *path) {
  slist_t *dirlst = directory_list(path);
  return dirlst;
}
