#include <stdio.h>
#include <errno.h>
#include "directory.h"
#include <string.h>
#include <assert.h>
#include "storage.h"

//initiate directory
void directory_init() {
	inode_t *nd = get_inode(alloc_inode());
	// set the mode to the directory to 040755
	nd->mode = 040755;  
}

//look up direcrory
int directory_lookup(inode_t *dd, const char *name) {
  // get the block using our node and then loop through it to find a file
  dirent_t *blklst = blocks_get_block(dd->block);
  for (int i = 0; i < 4096 / sizeof(dirent_t); i++, 
	blklst = (void*)blklst + sizeof (dirent_t)) {
    if(strcmp(blklst->name, name) == 0) {
	   return blklst->inum;
	  }
  }
  return -ENOENT;
}

// returns the inum given a path
int tree_lookup(const char *path) {
	// we first check if there is a '/' at the beginning of the path
  int inum=0;
	assert(path[0] == '/');
	if(strcmp(path, "/") == 0){
	  return 0;
	}
	path+=1;
  slist_t *pathlst = s_explode(path, '/');
  //loop through the list and get the number
  while (pathlst != NULL) {
    inum = directory_lookup(get_inode(inum), pathlst->data);
    pathlst = pathlst->next;
  }
  
  return inum;
}
    
// add a new directory to dd, indexed by inum
int directory_put(inode_t *dd, const char *name, int inum) {
	// initialize a block list using dirent structure
  dirent_t* blklst = (dirent_t*) blocks_get_block(dd->block);	
	blklst += dd->size/DIR_NAME_LENGTH;
	// we copy the name into the list
	strncpy(blklst->name, name, strlen(name) + 1);
	// we add the inum to the list and also increase the size by the new entry
	blklst->inum = inum;
	dd->size += DIR_NAME_LENGTH;
	return 0;
}

// delete a directory
int directory_delete(inode_t *dd, const char *name) {
  int rv = 0;
  int rm = 0;
  // we get our block list
  dirent_t* blklst = (dirent_t*) blocks_get_block(dd->block);
  // loop through the block list and then check to see if we find the name
  for (int i = 0; i < 4096 / sizeof(dirent_t); i++) {
		if(!(strcmp(blklst[i].name, name))) {
    	rm = i;
    memcpy(&blklst[rm], &blklst[rm + 1], 4096 - ((rm + 1) * sizeof(dirent_t)));
    return rv;
		}
	}
}


// return the list of the directory 
slist_t *directory_list(const char* path) {
  // we get the node given our path and create a block list using that node
  inode_t *dd = get_inode(tree_lookup(path));  
  dirent_t *blklst = blocks_get_block(dd->block);
  // we initalize a size here so its easy to just set as the limit of our for loop
  int size =dd->size/sizeof(dirent_t);

  // generate directory list by looping through and adding every
  // value from our block list to the directory list
  slist_t *dirlst = 0; 
  for (int i = 0; i < size; ++i) {
    dirlst = s_cons(blklst[i].name, dirlst);
  }
  return dirlst;
}

//print the directory
void print_directory(inode_t *dd) {
  dirent_t *blklst = blocks_get_block(dd->block);
  int size =dd->size/sizeof(dirent_t);
   for (int i = 0; i < size; ++i) {
     printf("%s\n", blklst[i].name);
   }
}


