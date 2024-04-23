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
#define MAX_PATH_LEN ((size_t) 4096)
#define SIZE_BLOCK ((size_t) 1024)

/* We cannot store pointers, only how far something is from
   the start of the memory region.
*/
typedef size_t __myfs_off_t;

// ---------------------------------------------------------------------------------
// STRUCTS

/* We define a filesystem handler */
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
  
typedef struct __myfs_free_memory_block_struct_t __myfs_free_memory_block_t;


/* This struct represents a simple linked list structure */
struct __myfs_linked_list_struct_t {
  __myfs_off_t first_space;
};

typedef struct __myfs_linked_list_struct_t __myfs_linked_list_t;

// END OF STRUCTS
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

void *get_free_memory_ptr(void *fsptr) {
  return &((__myfs_handler_t *)fsptr)->free_memory;
}

char *get_last_token(const char *path, unsigned long *token_len) {
  unsigned long len = strlen(path);
  unsigned long index;

  // Find last '/' in path, worst case scenario, the first '/' is the root
  // directory at idx = 0
  for (index = len - 1; index > 0; index--) {
    if (path[index] == '/') break;
  }

  // index is at '/' but we want to start a character after it
  index++;

  // Set the length of the last node
  *token_len = len - index;

  // Make a copy of the last token
  void *ptr = malloc((*token_len + 1) * sizeof(char));

  // Check that malloc was successful
  if (ptr == NULL) {
    return NULL;
  }

  char *copy = (char *)ptr;
  strcpy(copy, &path[index]);  // strcpy(dst,src)

  // Add null character to terminate the string
  copy[*token_len] = '\0';

  return copy;
}

// ---------------------------------------------------------------------------------
// MEMORY ALLOCATION FUNCTIONS


/* Make an memory block item using length and start and add it to the list which
 * does it in ascending order */
void add_allocation_space(void *fsptr,  __myfs_linked_list_t *LL, __myfs_free_memory_block_t *alloc) {
  __myfs_free_memory_block_t *temp;
  __myfs_off_t temp_off = LL->first_space;
  __myfs_off_t alloc_off = __myfs_ptr_to_offset(fsptr, alloc);

  // New space address is less than the first_space in LL
  if (temp_off > alloc_off) {
    /* At this point we know that alloc comes before LL->first_space */
    LL->first_space = alloc_off;  // Update first space
    // Check if we can merge LL->first_space and alloc
    if ((alloc_off + sizeof(size_t) + alloc->remaining_size) == temp_off) {
      // Get first pointer
      temp = __myfs_offset_to_ptr(fsptr, temp_off);
      // Combine the spaces available
      alloc->remaining_size += sizeof(size_t) + temp->remaining_size;
      // Update pointers
      alloc->next_space = temp->next_space;
    }
    // We couldn't merge so we just add it as the first_space and update
    // pointers
    else {
      alloc->next_space = temp_off;
    }
  }
  // Find after what pointer does alloc should be added
  else {
    temp = __myfs_offset_to_ptr(fsptr, temp_off);
    // Get the last pointer that is in lower memory than alloc
    while ((temp->next_space != 0) && (temp->next_space < alloc_off)) {
      temp = __myfs_offset_to_ptr(fsptr, temp->next_space);
    }
    temp_off = __myfs_ptr_to_offset(fsptr, temp);

    // At this point, temp_off < alloc_off < temp->next_space. But, there is no
    // guaranty that temp->next_space != 0 (NULL)

    // If temp->next_space != 0 we make alloc_off to point to it and try to
    // merge them
    __myfs_off_t after_alloc_off = temp->next_space;
    if (after_alloc_off != 0) {
      // Check if we can merge alloc and after_alloc
      if ((alloc_off + sizeof(size_t) + alloc->remaining_size) == after_alloc_off) {
        __myfs_free_memory_block_t *after_alloc = __myfs_offset_to_ptr(fsptr, after_alloc_off);
        alloc->remaining_size += sizeof(size_t) + after_alloc->remaining_size;
        alloc->next_space = after_alloc->next_space;
      }
      // We couldn't merge them
      else {
        alloc->next_space = after_alloc_off;
      }
    }
    // alloc is the last space available in memory ascending order
    else {
      alloc->next_space = 0;
    }
    // Try to merge temp and alloc
    if ((temp_off + sizeof(size_t) + temp->remaining_size) == alloc_off) {
      temp->remaining_size += sizeof(size_t) + alloc->remaining_size;
      temp->next_space = alloc->next_space;
    }
    // We couldn't merge them
    else {
      temp->next_space = alloc_off;
    }
  }
}

void extend_pref_block(void *fsptr,   __myfs_free_memory_block_t *before_pref,
                       __myfs_free_memory_block_t *org_pref, __myfs_off_t pref_off,
                       size_t *size) {
  __myfs_free_memory_block_t *pref = __myfs_offset_to_ptr(fsptr, pref_off);
  __myfs_free_memory_block_t *temp;

  // Check if you can get all size bytes from the prefer block
  if (pref->remaining_size >= *size) {
    // Check if we can make an AllocateFrom object with the remaining space
    if (pref->remaining_size > *size + sizeof(__myfs_free_memory_block_t)) {
      // Make the new AllocateFrom object
      temp = ((void *)pref) + *size;
      temp->remaining_size = pref->remaining_size - *size;
      temp->next_space = pref->next_space;

      // Set original pref with final total size
      org_pref->remaining_size += *size;

      // Update pointers to add temp into list of free blocks
      before_pref->next_space = pref_off + *size;
    }
    // We can't make an AllocateFrom object
    else {
      // Add everything that the prefer block have into the original one
      org_pref->remaining_size += pref->remaining_size;

      // Update pointers so the one that was pointing to the prefer free block
      // is now pointing to the next free
      before_pref->next_space = pref->next_space;
    }
    *size = ((__myfs_off_t)0);
  }
  // We couldn't got everything from the prefer block so we get as much as we
  // can from it
  else {
    // Add everything that the prefer block have into the original one
    org_pref->remaining_size += pref->remaining_size;

    // Update pointers so the one that was pointing to the prefer free block is
    // now pointing to the next free
    before_pref->next_space = pref->next_space;

    // Update size because we have gotten some space
    *size -= pref->remaining_size;
  }
}

/* Check if the offset for pref_ptr is 0, if so we get any block for size,
 * otherwise we try to find the block after it and get as much from it as
 * possible and get the rest from the largest block
 */
void *get_allocation(void *fsptr, __myfs_linked_list_t *LL,
		     __myfs_free_memory_block_t *org_pref, size_t *size) {
  // There is no guarantee that its offset is not 0, if so, we don't consider it
  __myfs_off_t pref_off = ((__myfs_off_t)0);

  // Before current space
  __myfs_off_t before_temp_off;
  __myfs_free_memory_block_t *before_temp;

  // current space
  __myfs_off_t temp_off;
  __myfs_free_memory_block_t *temp;

  // Largest block variables
  __myfs_free_memory_block_t *before_largest = NULL;
  __myfs_off_t largest_off;
  __myfs_free_memory_block_t *largest;
  size_t largest_size;

  // Use this ptr if a new block needs to be return
  __myfs_free_memory_block_t *ptr = NULL;

  // Save first space
  before_temp_off = LL->first_space;

  // If before_temp have an offset of zero we have use all possible space in
  // memory
  if (!before_temp_off) {
    return NULL;
  }

  // Check that size is at least a sizeof(__myfs_off_t)
  if (*size < sizeof(__myfs_free_memory_block_t)) {
    *size = sizeof(__myfs_free_memory_block_t);
  }

  if (((void *)org_pref) != fsptr) {
    pref_off =
        __myfs_ptr_to_offset(fsptr, org_pref) + sizeof(size_t) + org_pref->remaining_size;
    // Check that the first block is the prefer one or not
    if (pref_off == before_temp_off) {
      extend_pref_block(fsptr, ((void *)LL) - sizeof(size_t), org_pref,
                        pref_off, size);
      if (*size == ((size_t)0)) {
        return NULL;
      }
    }
  }

  // We currently have before_temp as our largest block
  before_temp_off = LL->first_space;
  before_temp = __myfs_offset_to_ptr(fsptr, before_temp_off);
  largest_off = before_temp_off;
  largest = before_temp;
  largest_size = before_temp->remaining_size;

  // Get next space
  temp_off = before_temp->next_space;

  // Iterate the list until the first block that can hold size and the block
  // after pref_ptr is found (or pass because it is not there)
  while (temp_off != ((__myfs_off_t)0)) {
    // The offset is to get after the size_t so we readjust it
    temp = __myfs_offset_to_ptr(fsptr, temp_off);
    // Check if temp_off is the prefer block that we are looking for or if temp
    // have more space available than the previous largest
    if ((pref_off == temp_off) || (temp->remaining_size > largest_size)) {
      // If temp_off is pref we would not like to update largest space so we
      // have two places to get space from
      if (pref_off == temp_off) {
        // pref_found was successfully found
        extend_pref_block(fsptr, before_temp, org_pref, pref_off, size);
        if (size == ((__myfs_off_t)0)) {
          break;
        }
      }
      // Update largest space
      else {
        before_largest = before_temp;
        largest_off = temp_off;
        largest = temp;
        largest_size = temp->remaining_size;
      }
    }
    // Update pointers to next space
    before_temp_off = temp_off;
    before_temp = temp;
    temp_off = temp->next_space;
  }

  // If size is still not 0 we get as much as we can from it or until size is 0
  if ((*size != ((__myfs_off_t)0)) && (largest != NULL)) {
    ptr = largest;
    // Check if the largest block can give everything that we are missing
    if (largest->remaining_size >= *size) {
      // Check if we can make an memory block object after getting size bytes
      // from it
      if (largest->remaining_size > *size + sizeof(__myfs_free_memory_block_t)) {
        // Make the new memory block object
        temp = ((void *)largest) + sizeof(size_t) + *size;  
        temp->remaining_size = largest->remaining_size - *size - sizeof(size_t);
        temp->next_space = largest->next_space;
        // Update before_largest pointer, it may be the LL or a memory block
        if (before_largest == NULL) {
          // before_largest is the LL
          LL->first_space = largest_off + *size + sizeof(size_t);
        } else {
          // Update pointers to add temp list of free blocks
          before_largest->next_space = largest_off + *size + sizeof(size_t);
        }
        // Set original pref with final total size
        ptr->remaining_size = *size;
      }
      // We can't make an memory block object so we get everything
      else {
        // Use everything that the largest block have
        if (before_largest != NULL) {
          before_largest->next_space = largest->next_space;
        } else {
          // Because largest is the first block and we use everything, it means
          // that the system have no memory left
          LL->first_space = ((__myfs_off_t)0);
        }
      }
      *size = ((__myfs_off_t)0);
    }
    // We couldn't got everything from the largest block so we get as much as we
    // can from it
    else {
      // Update pointers
      if (before_largest == NULL) {
        // We had use everything and we still didn't got enough size
        return NULL;
      } else {
        before_largest->next_space = largest->next_space;
        // Update size
        *size -= largest->remaining_size;
      }
    }
  }

  // We suppose to return the new block address if a new block outside the
  // prefer one was needed
  if (ptr == NULL) {
    return NULL;
  }
  return ((void *)ptr) + sizeof(size_t);  // ptr was set to the largest which is
                                          // at the size_t header so we resize
}

/* If size is zero, return NULL. Otherwise, call get_allocation with size. */
void *__malloc_impl(void *fsptr, void *pref_ptr, size_t *size) {
  if (*size == ((size_t)0)) {
    return NULL;
  }

  // If we don't have a prefer pointer we set it to be sizeof(size_t) so the
  // get_allocation pointer is 0 again
  if (pref_ptr == NULL) {
    pref_ptr = fsptr + sizeof(size_t);
  }

  return get_allocation(fsptr, get_free_memory_ptr(fsptr),
                        pref_ptr - sizeof(size_t), size);
}

/* Add space back to List using add_allocation_space */
void __free_impl(void *fsptr, void *ptr) {
  if (ptr == NULL) {
    return;
  }
  // Adjust ptr by size_t to start on the size of pointer
  add_allocation_space(fsptr, get_free_memory_ptr(fsptr), ptr - sizeof(size_t));
}

/* If size is less than what already assign to *ptr just lock what is after size
 * and add it using add_allocation_space. */
void *__realloc_impl(void *fsptr, void *orig_ptr, size_t *size) {
  // If size is 0, we free the ptr and return NULL
  if (*size == ((size_t)0)) {
    __free_impl(fsptr, orig_ptr);
    return NULL;
  }

  __myfs_linked_list_t *LL = get_free_memory_ptr(fsptr);

  // If ptr is fsptr if the offset was 0 (kind of pointing to null), realloc()
  // is identical to a call to malloc() for size bytes.
  if (orig_ptr == fsptr) {
    // fsptr because we don't have a preference over the location of the pointer
    // that would be returned, (offset of 0).
    return get_allocation(fsptr, LL, fsptr, size);
  }

  __myfs_free_memory_block_t *alloc = (__myfs_free_memory_block_t *)(((void *)orig_ptr)
								     - sizeof(size_t));
  __myfs_free_memory_block_t *temp;
  void *new_ptr = NULL;

  // If the new size is less than before but not enough to make an AllocateFrom
  // object
  if ((alloc->remaining_size >= *size) &&
      (alloc->remaining_size < (*size + sizeof(__myfs_free_memory_block_t)))) {
    // No new ptr was created
    new_ptr = orig_ptr;
  }
  // If the new size is less than before and we can create an AllocateFrom
  // element to add to LL
  else if (alloc->remaining_size > *size) {
    // Save what is left in temp and add it to the LL
    temp = (__myfs_free_memory_block_t *)(orig_ptr + *size);
    temp->remaining_size = alloc->remaining_size - *size - sizeof(size_t);
    temp->next_space = 0;  // Offset of zero
    add_allocation_space(fsptr, LL, temp);
    // Update remaining space
    alloc->remaining_size = *size;
    // No new ptr was created
    new_ptr = orig_ptr;
  }
  // If we are asking for more than what we have in alloc
  else {
    // Get new space to copy to
    new_ptr = get_allocation(fsptr, LL, fsptr, size);
    // We couldn't get enough space
    if (*size != 0) {
      return NULL;
    }
    // Copy what was inside orig_ptr into new_ptr
    memcpy(new_ptr, orig_ptr, alloc->remaining_size);  // memcpy(dst,src,len)
    // Free the space of the original pointer
    add_allocation_space(fsptr, LL, alloc);
  }

  return new_ptr;
}

// END OF MEMORY ALLOCATION FUNCTIONS
// ---------------------------------------------------------------------------------

/* This function updates the time information for a given node */
void update_time( __myfs_node_t *node, int set_mod) {
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
  // Typecast fsptr
  __myfs_handler_t *handler = ((__myfs_handler_t *)fsptr);

  // If the handler's magic number does not match the magic number
  // constant we have, it means that we need to mount the file system
  // for the first time
  if (handler->magic_number != MYFS_MAGIC) {
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
    __myfs_free_memory_block_t *free_memory_block = (__myfs_offset_to_ptr(fsptr,
									   linked_list->first_space));

    // Set everything to zero
    free_memory_block->remaining_size = fssize - handler->free_memory - sizeof(size_t);
    memset(((void *)free_memory_block) + sizeof(size_t), 0,
	   free_memory_block->remaining_size);
  }
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


__myfs_node_t *add_node(void *fsptr, const char *path, int *errnoptr, int isfile) {
  
  __myfs_node_t *parent_node = NULL;

  // Make a copy of the path to preserve the original
  char path_copy[MAX_PATH_LEN]; // Assuming MAX_PATH_LENGTH is defined appropriately
  strcpy(path_copy, path);

  // Find the last occurrence of '/' in the path string
  char *last_slash = strrchr(path_copy, '/');
  if (last_slash != NULL) {
    // Null-terminate the path copy at the last slash to remove the last component
    *last_slash = '\0';
    // Call follow_path with the modified path copy (now excluding the last component)
    parent_node = follow_path(fsptr, path_copy);
  }

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
  __myfs_directory_node_t *dir = &parent_node->type.directory_node;

  // Get last token which has the filename
  unsigned long len;
  char *new_node_name = get_last_token(path, &len);

  // Check that the parent doesn't contain a node with the same name as the one
  // we are about to create
  if (get_node(fsptr, dir, new_node_name) != NULL) {
    *errnoptr = EEXIST;
    return NULL;
  }

  if (len == 0) {
    *errnoptr = ENOENT;
    return NULL;
  }

  if (len > MAX_LEN_NAME) {
    *errnoptr = ENAMETOOLONG;
    return NULL;
  }

  __myfs_off_t *children = __myfs_offset_to_ptr(fsptr, dir->children);
  __myfs_free_memory_block_t *block = (((void *)children) - sizeof(size_t));

  // Make the node and put it in the directory child list
  // First check if the directory list have free places to added nodes to
  // Amount of memory allocated doesn't count the sizeof(size_t) as withing
  // the available size
  size_t max_children = (block->remaining_size) / sizeof(__myfs_off_t);
  size_t ask_size;
  if (max_children == dir->number_children) {
    ask_size = block->remaining_size * 2;
    // Make more space for another children
    void *new_children = __realloc_impl(fsptr, children, &ask_size);

    //__realloc_impl() always returns a new pointer if ask_size == 0, otherwise
    //we don't have enough space in memory
    if (ask_size != 0) {
      *errnoptr = ENOSPC;
      return NULL;
    }

    // Update offset to access the children
    dir->children = __myfs_ptr_to_offset(fsptr, new_children);
    children = ((__myfs_off_t *)new_children);
  }

  ask_size = sizeof(__myfs_node_t);
  __myfs_node_t *new_node = (__myfs_node_t *)__malloc_impl(fsptr, NULL, &ask_size);
  if ((ask_size != 0) || (new_node == NULL)) {
    __free_impl(fsptr, new_node);
    *errnoptr = ENOSPC;
    return NULL;
  }
  memset(new_node->name, '\0',
         MAX_LEN_NAME + ((size_t)1));  // File all name characters to '\0'
  memcpy(new_node->name, new_node_name,
         len);  // Copy given name into node->name, memcpy(dst,src,n_bytes)
  update_time(new_node, 1);

  // Add file node to directory children
  children[dir->number_children] = __myfs_ptr_to_offset(fsptr, new_node);
  dir->number_children++;
  update_time(parent_node, 1);

  if (isfile) {
    // Make a node for the file with size of 0
    new_node->is_file = 1;
    __myfs_file_node_t *file = &new_node->type.file_node;
    file->total_size = 0;
    file->first_file_block = 0;
  } else {
    // Make a node for the file with size of 0
    new_node->is_file = 0;
    dir = &new_node->type.directory_node;
    dir->number_children =
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
    dir->children = __myfs_ptr_to_offset(fsptr, ptr);
    // Set first children to point to its parent
    *ptr = __myfs_ptr_to_offset(fsptr, parent_node);
  }

  return new_node;
} 


void free_file_info(void *fsptr, __myfs_file_node_t *file) {
  
  __myfs_file_block_t *block = __myfs_offset_to_ptr(fsptr, file->first_file_block);
  __myfs_file_block_t *next;

  // Iterate over all blocks until a block is pointing to fsptr meaning that we
  // are done
  while (((void *)block) != fsptr) {
    // Free block data information
    __free_impl(fsptr, __myfs_offset_to_ptr(fsptr, block->data));
    // Save next block pointer
    next = __myfs_offset_to_ptr(fsptr, block->next_file_block);
    // Free current block
    __free_impl(fsptr, block);
    // Update current block with the next file_t for the next iteration
    block = next;
  }
}

void remove_node(void *fsptr, __myfs_directory_node_t *dir, __myfs_node_t *node) {
  // Iterate over the files in dict and remove the file_node which we assume to
  // be already free by calling free_file_info with &file_node->type.file
  size_t n_children = dir->number_children;
  __myfs_off_t *children = __myfs_offset_to_ptr(fsptr, dir->children);
  size_t index;
  __myfs_off_t node_off = __myfs_ptr_to_offset(fsptr, node);

  // Find the index where the node is at
  for (index = 1; index < n_children; index++) {
    if (children[index] == node_off) {
      break;
    }
  }

  // File must be at index
  __free_impl(fsptr, node);

  // Move the remaining nodes one to the left to cover the node remove
  for (; index < n_children - 1; index++) {
    children[index] = children[index + 1];
  }

  // Set the last to have offset of zero and update number of children
  children[index] = ((__myfs_off_t)0);
  dir->number_children--;

  // See if we can free some memory by half while keeping at least 4 offsets
  size_t new_n_children =
      (*((size_t *)children) - 1) /
      sizeof(__myfs_off_t);  // Get the maximum number of children offset
  new_n_children <<= 1;      // Divide the maximum number by two

  // Check if the new number of children is greater or equal than the current
  // number, check that we always have 4 or more children spaces
  //  and that we can actually made an AllocateFrom object before making it
  if ((new_n_children >= dir->number_children) &&
      (new_n_children * sizeof(__myfs_off_t) >= sizeof(__myfs_free_memory_block_t)) &&
      (new_n_children >= 4)) {
    // Every condition is meet, so we proceed to make an AlloacteFrom object and
    // sent it to be added into the linked list of free blocks
    __myfs_free_memory_block_t *temp = ((__myfs_free_memory_block_t *)&children[new_n_children]);
    temp->remaining_size = new_n_children * sizeof(__myfs_off_t) - sizeof(size_t);
    temp->next_space = 0;
    __free_impl(fsptr, temp);

    // Update the new size of the current directory children array of offsets
    size_t *new_size = (((size_t *)children) - 1);
    *new_size -= (temp->remaining_size - sizeof(size_t));
  }
}

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

  // Check that directory has more than children "." and ".." inside 
  __myfs_directory_node_t *dir = &node->type.directory_node;
  if (dir->number_children == 1) {
    return 0;
  }

  size_t n_children = dir->number_children;
  // Allocate space for all children, excluding "." and ".."
  void **ptr = (void **)calloc(n_children - ((size_t)1), sizeof(char *));
  __myfs_off_t *children = __myfs_offset_to_ptr(fsptr, dir->children);

  // Check that calloc call was successful
  if (ptr == NULL) {
    *errnoptr = EINVAL;
    return -1;
  }

  char **names = ((char **)ptr); // Array of names
  size_t len;
  for (size_t i = ((size_t)1); i < n_children; i++) {
    node = ((__myfs_node_t *)__myfs_offset_to_ptr(fsptr, children[i]));
    len = strlen(node->name);
    names[i - 1] = (char *)malloc(len + 1);
    // Copy node->name into names array
    strcpy(names[i - 1], node->name);  // strcpy(dst,src)
    names[i - 1][len] = '\0';
  }

  *namesptr = names;
  return ((int)(n_children - 1)); // Return number of children
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
  
  // Initialize the file system if necessary
  initialize_file_system_if_necessary(fsptr, fssize);

  // Make a node of type file
  __myfs_node_t *node = add_node(fsptr, path, errnoptr, 1);

  if (node == NULL) {
    // Node was not created successfully
    return -1;
    } 

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
    
  // Initialize the file system if necessary
  initialize_file_system_if_necessary(fsptr, fssize);

  __myfs_node_t *parent_node = NULL;

  // Make a copy of the path to preserve the original
  char path_copy[MAX_PATH_LEN]; // Assuming MAX_PATH_LENGTH is defined appropriately
  strcpy(path_copy, path);

  // Find the last occurrence of '/' in the path string
  char *last_slash = strrchr(path_copy, '/');
  if (last_slash != NULL) {
    // Null-terminate the path copy at the last slash to remove the last component
    *last_slash = '\0';
    // Call follow_path with the modified path copy (now excluding the last component)
    parent_node = follow_path(fsptr, path_copy);
  }

  if (parent_node == NULL) {
    // Parent file does not exist
    *errnoptr = ENOENT;  // "No such file or directory"
    return -1;
  }

  // Check that the node returned is a directory
  if (parent_node->is_file) {
    *errnoptr = ENOTDIR;
    return -1; // "Not a directory"
  }

  // Get directory from node
  __myfs_directory_node_t *dir = &parent_node->type.directory_node;

  // Get last token which has the filename
  unsigned long len;
  char *filename = get_last_token(path, &len);

  // Check that the parent don't contain a node with the same name
  // as the one we are about to create
  __myfs_node_t *file_node = get_node(fsptr, dir, filename);

  if (file_node == NULL) {
    *errnoptr = ENOENT;
    return -1;
  }
  // Check that file_node is actually a file
  if (!file_node->is_file) {
    // Path given lead to a directory not a file
    *errnoptr = EISDIR;
    return -1;
  }

  // Free file information
  __myfs_file_node_t *file = &file_node->type.file_node;
  if (file->total_size != 0) {
    free_file_info(fsptr, file);
  }

  // Remove file_node from parent directory
  remove_node(fsptr, dir, file_node);

  return 0;
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
