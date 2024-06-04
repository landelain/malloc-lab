/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Very sane and mentaly stable individuals",
    /* First member's full name */
    "Angela Garcinuno Feliciano",
    /* First member's email address */
    "angela.garcinuno-feliciano@polytechnique.edu",
    /* Second member's full name (leave blank if none) */
    "Lucas Massot",
    /* Second member's email address (leave blank if none) */
    "lucas.massot@polytechnique.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* IDEAS:
 *	Coalsecing: merge free blocks that are adjacent in memory
 *
 *
 *
 */


/*
Use:
	Blocks:
		word 1 [boundary tag]: block size and allocated (001) / free (000) tag
	Segregated lists: different free lists for different size classes
		separate classes for each small size
		for larger sizes: one class for each two-power size
	Explicit Free lists: maintain lists of free block, not all blocks.
		We use the payload area of the free blocks to store backward and forward
		pointers. We have a linked list of free blocks.
	First fit: go through free list and place block in firt place it fits
	Free: pointer to the allocated block, also contains size. We can simply change the
	      boundary tag and add the backward and forward pointers. This should be easy
	      since we're using linked lists.

	We can maybe implement coalescing after having everything up to here working.
	Immediate coalescing: coalesce each time free is called (ideally in a function we can
			      later adapt to deffered coalescing)



	1st: make the segregated lists data structure. Since we can't have arrays as global
	     variables, we can use pointers to the first element of each list.
 */


void* start;


// our auxiallary function 

int merge_blocks();
int scan_list();
void* get_next(void* current); // in the memory
void* get_previous(void* current); // in the memory


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // maybe initialize a global variable linked list ? maybe two for allocated and free ?

	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

	// Ignore requests if size = 0
	if (size == 0){ return NULL;}


	// adjust block size to stay aligned (maybe we need to increase block size to account for 
	// boundary tag

	int newsize = ALIGN(size + SIZE_T_SIZE);

	(void *) firstelt; // list where we want to place block

	// search spot in heap to fit new block
	// Search appropriate free list for block of size m > n
	if (newsize <= 8){ firstelt = explicitSize8; }
	else if (newsize <= 16){ firstelt = explicitSize16; }
	else if (newsize <= 32){ firstelt = explicitSize32; }

	while (firstelt != NULL){
		// Check if size is greater than newsize.
		if (){

			unsigned char * allocated = ((unsigned char *)firstelt)+7; // last byte of the tag
			*allocated = 1;

			// Update boundary tag
			// remove elt from linked list
			return firstelt;
		}
		// update firstelt to next element of linked list
	}

	// if spot not found, place the block at the end, allocating
	// more memory to the heap if needed

//  Below is the basic implementation given in template file
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	//~check that the pointer really is one

	if (ptr != NULL){

		double *tag = (double*)ptr; // this is to get the full tag
		unsigned char *allocated = ((unsigned char *)firstelt)+7; // last byte of the word tag

		//~check if the pointer has been returned by a malloc or realloc before (that is is it allocated ?)
		if ( *allocated == 1){

			*allocated = 0; // modify Boundary tag (allocated part)

			// add to appropriate linked list
		}
	}
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

// Implementation given in template file. We don't work on realloc until we've
// correctly implemented malloc and free. 
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}



/*
 * mm_check - checks heap consistency
 */
int mm_checks(void)
{
	
}
