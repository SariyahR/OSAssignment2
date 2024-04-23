/*

  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Copyright 2022-24

  University of Texas at El Paso, Department of Computer Science.

  Contributors: Christoph Lauter 
                ... and
                ...

  and based on 

  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
// headers we added
#include <fuse/fuse.h>


/* The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.

   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.

   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.

   CAUTION:

   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.

   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.

   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.

   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.

   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!

   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:

   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.

   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.

   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.

   * And of course: your code MUST NOT SEGFAULT!

   It is reasonable to proceed in the following order:

   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinitialized at mount
         time if you initialized it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 

   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.

   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).
          
   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.

   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.

   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with 

         touch foo

         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.

   (7)   Design and implement __myfs_mkdir_implem. Test as above.

   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:

         truncate -s 1024 foo

   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.

   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).

   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.

   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:

         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo

         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.

   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.

   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.

   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.

   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.

   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).

   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.

*/



/* Helper types and functions */


/* Magic constant that tell us if the memory chunk has been
   initialized to a filesystem or not
*/
#define MYFS_MAGIC ((uint32_t) 0xdeadbeef)
#define MAX_LEN_NAME ((size_t) 255)
#define SIZE_BLOCK ((size_t) 1024)

/* We cannot store pointers, only how far something is from
   the start of the memory region.
*/
typedef size_t __myfs_off_t;

// ---------------------------------------------------------------------------------
// STRUCTS

/* In order to memory management of the memory chunk we have for the
   filsystem, we defie a header for the memory blocks
*/
struct __myfs_memory_block_struct_t {
  size_t size;
  size_t user_size;
  __myfs_off_t next;
};

typedef struct __myfs_memory_block_struct_t __myfs_memory_block_t;

/* Now we define a filesystem handler */
struct __myfs_handler_struct_t {
  uint32_t magic_number;
  __myfs_off_t free_memory;
  __myfs_off_t root_dir;
  size_t size;
};

typedef struct __myfs_handler_struct_t __myfs_handler_t;

/* This is the struct that defines a node in the tree for a directory.
   It has children but no data.
*/
struct __myfs_directory_node_struct_t {
  size_t number_children;
  // This is an array of offsets to other directories and files
  __myfs_off_t children;
};

typedef struct __myfs_directory_node_struct_t __myfs_directory_node_t;

/* This is the struct that defines a memory block for a file */
struct __myfs_file_block_struct_t {
  size_t size;
  size_t allocated;
  __myfs_off_t data;
  __myfs_off_t next_file_block;
};
  
typedef struct __myfs_file_block_struct_t __myfs_file_block_t;  

/* This is the struct that defines a node in the tree or a file.
   It has data but no children.
*/
struct __myfs_file_node_struct_t {
  size_t total_size;
  // This is an offset to the first file_block_t
  __myfs_off_t first_file_block; 
};

typedef struct __myfs_file_node_struct_t __myfs_file_node_t;

/* We have nodes for files and for directories.
   So we define a general node structure to encapsulte both.
*/
struct __myfs_node_struct_t {
  char name[MAX_LEN_NAME + ((size_t)1)]; // Both directories and files have a name
  char is_file;
  struct timespec 
    times[2];
  union {
    __myfs_directory_node_t directory_node;
    __myfs_file_node_t file_node;
  } type;
};

typedef struct __myfs_node_struct_t __myfs_node_t;

/* This struct represents an entity used for managing memory
   allocation within the file system
*/
struct __myfs_free_memory_block_struct_t {
  size_t remaining_size;
  __myfs_off_t next_space;
};
  
typedef struct __myfs_free_memory_block_struct_t __my_fs_free_memory_block_t; 

/* This struct represents a simple linked list structure */
struct __myfs_linked_list_struct_t {
  __myfs_off_t first_space;
};

typedef struct __myfs_linked_list_struct_t __myfs_linked_list_t;

// ---------------------------------------------------------------------------------

/* Converts offset to pointer
   fsptr: pointer to the root of the tree
   off: offset we want to convert
*/
static inline void * __myfs_offset_to_ptr(void *fsptr, __myfs_off_t off) {
  void *ptr = fsptr + off;
  // Check overflow
  if (ptr < fsptr) {
    return NULL;
  }
  return ptr;
} 

/* Converts pointer to offset.
     fsptr: pointer to the root of the tree
     ptr: pointer we want to convert
*/
static inline __myfs_off_t __myfs_ptr_to_offset(void *fsptr, void *ptr) {
  if (fsptr > ptr) {
    return 0;
  }
  return ((__myfs_off_t)(ptr - fsptr));
}

/* This function updates the time information for a given node */
void update_time( __myfs_node_t *node, int set_mod) {
  printf("Hellooooooo 3");
  if (node == NULL) {
    return;
  }

  struct timespec ts;

  if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
    node->times[0] = ts;
    if (set_mod) {
      node->times[1] = ts;
    }
  }
}

/* This function checks that the file system is initialized and
   if not, it initializes it
*/
void initialize_file_system_if_necessary(void *fsptr, size_t fssize) {
  printf("Hellooooooo 2");
  // Typecast fsptr
  __myfs_handler_t *handler = ((__myfs_handler_t *)fsptr);

  // If the handler's magic number does not match the magic number
  // constant we have, it means that we need to mount the file system
  // for the first time
  if (handler->magic_number == MYFS_MAGIC) {
    printf("Inside no magic number if statement");
    // Set basic handler struct attributes
    handler->magic_number = MYFS_MAGIC;
    handler->size = fssize;
    
    // The root directory comes right after the handler and it is the
    // beginning of our file system
    // Save the offset to the root directory in out handler struct
    handler->root_dir = sizeof(__myfs_handler_t);
    __myfs_node_t *root = __myfs_offset_to_ptr(fsptr, handler->root_dir);

    // Set the name of root directory as '/'
    // It has 4 children
    memset(root->name, '\0', MAX_LEN_NAME + ((size_t)1)); 
    memcpy(root->name, "/", strlen("/"));  // memcpy(dst,src,n_bytes)
    update_time(root, 1);
    root->is_file = 0; 
    __myfs_directory_node_t *dir_node = &root->type.directory_node;
    dir_node->number_children = ((size_t)1);  // The first child space is '..'

    // Root children start after the root node and we first set the header of
    // the block with the amount of children
    dir_node->children =
      __myfs_ptr_to_offset(fsptr, ((void *)(4 * sizeof(__myfs_off_t))) + sizeof(size_t));

    // Set the handler free_memory pointer
    handler->free_memory = dir_node->children +  (4 * sizeof(__myfs_off_t));
    __myfs_linked_list_t *linked_list = (__myfs_linked_list_t *)&((__myfs_handler_t *)fsptr)->free_memory;
    __my_fs_free_memory_block_t *free_memory_block = (__myfs_offset_to_ptr(fsptr,
									   linked_list->first_space));

    // Set everything to zero
    free_memory_block->remaining_size = fssize - handler->free_memory - sizeof(size_t);
    memset(((void *)free_memory_block) + sizeof(size_t), 0,
	   free_memory_block->remaining_size);
  }
  printf("After print statement");
}

/* Retrieves a child node (file or directory) from a directory node. */
__myfs_node_t *get_node(void *fsptr, __myfs_directory_node_t *dir,
			const char *child) {
  size_t total_children = dir->number_children;
  __myfs_off_t *children = __myfs_offset_to_ptr(fsptr, dir->children);
  __myfs_node_t *node = NULL;

  if (strcmp(child, "..") == 0) {
    //Go to the parent directory
    return ((__myfs_node_t *)__myfs_offset_to_ptr(fsptr, children[0]));
  }

  // We start at i = 1 since the first child is ".." (parent)
  for (size_t i = ((size_t)1); i < total_children; i++) {
    node = ((__myfs_node_t *)__myfs_offset_to_ptr(fsptr, children[i]));
    if (strcmp(node->name, child) == 0) {
      return node;
    }
  }
  return NULL;
}

/* This function resolves a path to the corresponding node within the
   file system, handling different cases such as invalid paths and
   files not having children.
*/
__myfs_node_t *follow_path(void *fsptr, const char *path) {
  // Path must start at root directory '/'
  if (*path != '/') {
    return NULL;
  }

  // Get root directory
  __myfs_node_t *node = __myfs_offset_to_ptr(fsptr, ((__myfs_handler_t *)fsptr)->root_dir);

  // If the path is only '/', we return the root dir
  if (path[1] == '\0') {
    return node;
  }

  // Make a mutable copy of the path
  char *path_copy = strdup(path);
  if (path_copy == NULL) {
    // Handle memory allocation failure
    return NULL;
  }

  // Tokenize the path using strtok
  char *token = strtok(path_copy, "/");
  while (token != NULL) {
    // Files cannot have children
    if (node->is_file) {
      free(path_copy); // Free the memory allocated for the copy
      return NULL;
    }
    // If token is "." we stay on the same directory
    if (strcmp(token, ".") != 0) {
      node = get_node(fsptr, &node->type.directory_node, token);
      // Check that the child was successfully retrieved
      if (node == NULL) {
        free(path_copy); // Free the memory allocated for the copy
        return NULL;
      }
    }
    // Get the next token
    token = strtok(NULL, "/");
  }

  // Free the memory allocated for the copy
  free(path_copy);
  
  return node;
}

/*
__myfs_node_t *add_node(void *fsptr, const char *path, int *errnoptr, int isfile) {
  // Call path solver without the last node name because that is the file name
  // if valid path name is given
  node_t *parent_node = path_solver(fsptr, path);

  // Check that the file parent exist
  if (parent_node == NULL) {
    *errnoptr = ENOENT;
    return NULL;
  }

  // Check that the node returned is a directory
  if (parent_node->is_file) {
    *errnoptr = ENOTDIR;
    return NULL;
  }

  // Get directory from node
  directory_t *dict = &parent_node->type.directory;

  // Get last token which have the filename
  unsigned long len;
  char *new_node_name = get_last_token(path, &len);

  // Check that the parent doesn't contain a node with the same name as the one
  // we are about to create
  if (get_node(fsptr, dict, new_node_name) != NULL) {
    *errnoptr = EEXIST;
    return NULL;
  }

  if (len == 0) {
    *errnoptr = ENOENT;
    return NULL;
  }

  if (len > NAME_MAX_LEN) {
    *errnoptr = ENAMETOOLONG;
    return NULL;
  }

  __myfs_off_t *children = off_to_ptr(fsptr, dict->children);
  AllocateFrom *block = (((void *)children) - sizeof(size_t));

  // Make the node and put it in the directory child list
  //  First check if the directory list have free places to added nodes to
  //   Amount of memory allocated doesn't count the sizeof(size_t) as withing
  //   the available size
  size_t max_children = (block->remaining) / sizeof(__myfs_off_t);
  size_t ask_size;
  if (max_children == dict->number_children) {
    ask_size = block->remaining * 2;
    // Make more space for another children
    void *new_children = __realloc_impl(fsptr, children, &ask_size);

    //__realloc_impl() always returns a new pointer if ask_size == 0, otherwise
    //we don't have enough space in memory
    if (ask_size != 0) {
      *errnoptr = ENOSPC;
      return NULL;
    }

    // Update offset to access the children
    dict->children = ptr_to_off(fsptr, new_children);
    children = ((__myfs_off_t *)new_children);
  }

  ask_size = sizeof(node_t);
  node_t *new_node = (node_t *)__malloc_impl(fsptr, NULL, &ask_size);
  if ((ask_size != 0) || (new_node == NULL)) {
    __free_impl(fsptr, new_node);
    *errnoptr = ENOSPC;
    return NULL;
  }
  memset(new_node->name, '\0',
         NAME_MAX_LEN + ((size_t)1));  // File all name characters to '\0'
  memcpy(new_node->name, new_node_name,
         len);  // Copy given name into node->name, memcpy(dst,src,n_bytes)
  update_time(new_node, 1);

  // Add file node to directory children
  children[dict->number_children] = ptr_to_off(fsptr, new_node);
  dict->number_children++;
  update_time(parent_node, 1);

  if (isfile) {
    // Make a node for the file with size of 0
    new_node->is_file = 1;
    file_t *file = &new_node->type.file;
    file->total_size = 0;
    file->first_file_block = 0;
  } else {
    // Make a node for the file with size of 0
    new_node->is_file = 0;
    dict = &new_node->type.directory;
    dict->number_children =
        ((size_t)1);  // We use the first child space for '..'

    // Call __malloc_impl() to get enough space for 4 children
    ask_size = 4 * sizeof(__myfs_off_t);
    __myfs_off_t *ptr = ((__myfs_off_t *)__malloc_impl(fsptr, NULL, &ask_size));
    if ((ask_size != 0) || (ptr == NULL)) {
      __free_impl(fsptr, ptr);
      *errnoptr = ENOSPC;
      return NULL;
    }
    // Save the offset to get to the children
    dict->children = ptr_to_off(fsptr, ptr);
    // Set first children to point to its parent
    *ptr = ptr_to_off(fsptr, parent_node);
  }

  return new_node;
} */

/* End of helper functions */



/* Implements an emulation of the stat system call on the filesystem 
   of size fssize pointed to by fsptr. 
   
   If path can be followed and describes a file or directory 
   that exists and is accessable, the access information is 
   put into stbuf. 

   On success, 0 is returned. On failure, -1 is returned and 
   the appropriate error code is put into *errnoptr.

   man 2 stat documents all possible error codes and gives more detail
   on what fields of stbuf need to be filled in. Essentially, only the
   following fields need to be supported:

   st_uid      the value passed in argument
   st_gid      the value passed in argument
   st_mode     (as fixed values S_IFDIR | 0755 for directories,
                                S_IFREG | 0755 for files)
   st_nlink    (as many as there are subdirectories (not files) for directories
                (including . and ..),
                1 for files)
   st_size     (supported only for files, where it is the real file size)
   st_atim
   st_mtim

*/
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr,
                          uid_t uid, gid_t gid,
                          const char *path, struct stat *stbuf) {
  printf("Hellooooooo 1");
  
  // Initialize the file system if necessary
  initialize_file_system_if_necessary(fsptr, fssize);

  // Attempt to follow the path within the filesystem
  __myfs_node_t *node = follow_path(fsptr, path);

  if (node == NULL) {
    // Invalid path
    *errnoptr = ENOENT; // "No such file or directory"
    return -1;
  }

  stbuf->st_uid = uid; // Set user ID
  stbuf->st_gid = gid; // Set group ID

  if (node->is_file) {
    // If node represents a file
    stbuf->st_mode = S_IFREG; // Regular file
    stbuf->st_nlink = ((nlink_t)1); // One link for files
    stbuf->st_size = ((off_t)node->type.file_node.total_size); // File size
    stbuf->st_atim = node->times[0]; // Access time 
    stbuf->st_mtim = node->times[1]; // Modification time
  } else {
    // If node represents a directory
    stbuf->st_mode = S_IFDIR; // Directory
    __myfs_directory_node_t *dir = &node->type.directory_node;
    __myfs_off_t *children = __myfs_offset_to_ptr(fsptr, dir->children);
    stbuf->st_nlink = ((nlink_t)2); // Two links for directories
    for (size_t i = 1; i < dir->number_children; i++) {
      if (!((__myfs_node_t *)__myfs_offset_to_ptr(fsptr, children[i]))->is_file) {
	// Increment link count for each directory within this directory
        stbuf->st_nlink++;
      }
    }
  } 
  return 0;
}

/* Implements an emulation of the readdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   If path can be followed and describes a directory that exists and
   is accessable, the names of the subdirectories and files 
   contained in that directory are output into *namesptr. The . and ..
   directories must not be included in that listing.

   If it needs to output file and subdirectory names, the function
   starts by allocating (with calloc) an array of pointers to
   characters of the right size (n entries for n names). Sets
   *namesptr to that pointer. It then goes over all entries
   in that array and allocates, for each of them an array of
   characters of the right size (to hold the i-th name, together 
   with the appropriate '\0' terminator). It puts the pointer
   into that i-th array entry and fills the allocated array
   of characters with the appropriate name. The calling function
   will call free on each of the entries of *namesptr and 
   on *namesptr.

   The function returns the number of names that have been 
   put into namesptr. 

   If no name needs to be reported because the directory does
   not contain any file or subdirectory besides . and .., 0 is 
   returned and no allocation takes place.

   On failure, -1 is returned and the *errnoptr is set to 
   the appropriate error code. 

   The error codes are documented in man 2 readdir.

   In the case memory allocation with malloc/calloc fails, failure is
   indicated by returning -1 and setting *errnoptr to EINVAL.

*/
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr) {
  
  // Initialize the file system if necessary
  initialize_file_system_if_necessary(fsptr, fssize);

  // Attempt to follow the path within the filesystem
  __myfs_node_t *node = follow_path(fsptr, path);

  if (node == NULL) {
    // Invalid path 
    *errnoptr = ENOENT; // "No such file or directory"
    return -1;
  }

  if (node->is_file) {
    *errnoptr = ENOTDIR; // "Not a directory"
    return -1;
  }

  // Check that directory have more than 2 nodes "." and ".." inside 
  __myfs_directory_node_t *dir = &node->type.directory_node;
  if (dir->number_children == 1) {
    return 0;
  }

  size_t n_children = dir->number_children;
  // Allocate space for all children, except "." and ".."
  void **ptr = (void **)calloc(n_children - ((size_t)1), sizeof(char *));
  __myfs_off_t *children = __myfs_offset_to_ptr(fsptr, dir->children);

  // Check that calloc call was successful
  if (ptr == NULL) {
    *errnoptr = EINVAL;
    return -1;
  }

  char **names = ((char **)ptr);
  // Fill array of names
  size_t len;
  for (size_t i = ((size_t)1); i < n_children; i++) {
    node = ((__myfs_node_t *)__myfs_offset_to_ptr(fsptr, children[i]));
    len = strlen(node->name);
    names[i - 1] = (char *)malloc(len + 1);
    strcpy(names[i - 1], node->name);  // strcpy(dst,src)
    names[i - 1][len] = '\0';
  }

  *namesptr = names;
  return ((int)(n_children - 1));
}

/* Implements an emulation of the mknod system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the creation of regular files.

   If a file gets created, it is of size zero and has default
   ownership and mode bits.

   The call creates the file indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mknod.

*/
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /*
  // Initialize the file system if necessary
  initialize_file_system_if_necessary(fsptr, fssize);

  // Make a node of type file
  __myfs_node_t *node = add_node(fsptr, path, errnoptr, 1);

  if (node == NULL) {
    // Node was not created successfully
    return -1;
    } */

  return 0;
}

/* Implements an emulation of the unlink system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the deletion of regular files.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 unlink.

*/
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the rmdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call deletes the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The function call must fail when the directory indicated by path is
   not empty (if there are files or subdirectories other than . and ..).

   The error codes are documented in man 2 rmdir.

*/
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call creates the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mkdir.

*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the rename system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call moves the file or directory indicated by from to to.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   Caution: the function does more than what is hinted to by its name.
   In cases the from and to paths differ, the file is moved out of 
   the from path and added to the to path.

   The error codes are documented in man 2 rename.

*/
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the truncate system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call changes the size of the file indicated by path to offset
   bytes.

   When the file becomes smaller due to the call, the extending bytes are
   removed. When it becomes larger, zeros are appended.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 truncate.

*/
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the open system call on the filesystem 
   of size fssize pointed to by fsptr, without actually performing the opening
   of the file (no file descriptor is returned).

   The call just checks if the file (or directory) indicated by path
   can be accessed, i.e. if the path can be followed to an existing
   object for which the access rights are granted.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The two only interesting error codes are 

   * EFAULT: the filesystem is in a bad state, we can't do anything

   * ENOENT: the file that we are supposed to open doesn't exist (or a
             subpath).

   It is possible to restrict ourselves to only these two error
   conditions. It is also possible to implement more detailed error
   condition answers.

   The error codes are documented in man 2 open.

*/
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the read system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes from the file indicated by 
   path into the buffer, starting to read at offset. See the man page
   for read for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes read into the buffer is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 read.

*/
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the write system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes to the file indicated by 
   path into the buffer, starting to write at offset. See the man page
   for write for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes written into the file is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 write.

*/
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size, off_t offset) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the utimensat system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call changes the access and modification times of the file
   or directory indicated by path to the values in ts.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 utimensat.

*/
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]) {
  /* STUB */
  return -1;
}

/* Implements an emulation of the statfs system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call gets information of the filesystem usage and puts in 
   into stbuf.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 statfs.

   Essentially, only the following fields of struct statvfs need to be
   supported:

   f_bsize   fill with what you call a block (typically 1024 bytes)
   f_blocks  fill with the total number of blocks in the filesystem
   f_bfree   fill with the free number of blocks in the filesystem
   f_bavail  fill with same value as f_bfree
   f_namemax fill with your maximum file/directory name, if your
             filesystem has such a maximum

*/
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs* stbuf) {
  /* STUB */
  return -1;
}
