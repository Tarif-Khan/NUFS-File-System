#include <stdio.h>
#include <stdlib.h>

#include "inode.h"
#include "bitmap.h"


// print node
// prints the values of a node
void print_inode(inode_t *node) {
  printf("node->refs: %d\n", node->refs);
  printf("node->mode: %d\n", node->mode);
  printf("node->size: %d\n", node->size);
  printf("node->block: %d\n", node->block);
}

// get node by index
// gets the node using a node list with a bitmap
inode_t *get_inode(int inum) {
  //get the pointer to the inode table bitmap.
  inode_t *ndlst = get_inode_bitmap(); 
    
  if (ndlst !=NULL)
  	return &ndlst[inum];
  else
    return NULL;
}

//create a new node and add to bitmap
int alloc_inode() {
  void *bmap = get_inode_bitmap();
  int i=0;
  for (i = 0; i < BLOCK_COUNT; ++i) {
    if (bitmap_get(bmap, i)==0) {
      bitmap_put(bmap, i, 1);
      inode_t *nd = get_inode(i);
    	nd->refs = 1;
    	nd->size = 0;
      nd->mode = 0;
    	nd->block = alloc_block();
      break;
    }
  }
   
  return i;
}

// frees the inode from the bitmap
void free_inode(int inum) {
  inode_t *nd = get_inode(inum);
  void *bmap = get_inode_bitmap(); 
  //set bitmap flag to free
  bitmap_put(bmap, inum, 0);
}

// grows the indode given a size
int grow_inode(inode_t *node, int size) {
  node->size += size;
  return 0;
}

// shrink the inode given a size
int shrink_inode(inode_t *node, int size) {
  node->size = node->size - size;
  return 0;  
}

// get the inum given a node
int inode_get_bnum(inode_t *node, int fbnum) {
  return node->block;
}


