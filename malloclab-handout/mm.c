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

// big issue : we need to know if int are 8 or 4 bytes, or get a way around it by using unsigned char which are always 1 byte.

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


	feels like were assuming that were going for an alignement of 8 which is fine by me but lets be aware of that 
 */


void* start;



// our auxiallary functions

int merge_blocks();

void* find_smallest_fit(void* first, int size);
void* get_next(void* current);
void* get_previous(void* current);
int get_size(void* current);
int get_tag(void* current);
void set_next(void* current, void* next);
void set_previous(void* current, void* previous);
void set_size(void* current, int size);
void set_tag(void* current, int tag);
void remove_link(void* ptr);


void* find_smallest_fit(void* first, int size){
	void* current = first;
	void* best = NULL;
	int i = 0;
	fprintf(stderr, "Starting loop\n");
	while(current != NULL){
		++i;
		if (i == 10){ // ig that some sort of first instance checking counter ?
			break;
		}
		if (get_size(current) == size){							// If exact size
			fprintf(stderr, "Found exact size\n");
			return current;
		} else if (get_size(current) > size){						// If size ok
			fprintf(stderr, "Found ok size\n");
			if (best == NULL){										// Set best if Null
				fprintf(stderr, "Initialized best\n");
				best = current;
			} else if (get_size(best) > get_size(current)){		// Update best if better
				fprintf(stderr, "Updated best\n");
				best = current;
			}
		}
		fprintf(stderr, "Checking next\n");
		current = get_next(current);
	}
	return best;													// Return best fit
}

void* get_next(void* current){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return NULL;
	}

	if(!get_tag(current)){
		fprintf(stderr, "Error : not free block");
		return NULL;
	}

	int *header = (int *) current;
	if ((void *) header[1]){ // case were it doesnt have a next
		return NULL;
	}
	return (void *) header[1];
}

void* get_previous(void* current){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return NULL;
	}

	if(!get_tag(current)){
		fprintf(stderr, "Error : not free block");
		return NULL;
	}

	int *header = (int *) current;
	if ((void *) header[2]){ // case were it doesnt have a previous
		return NULL;
	}
	return (void *) header[2];
}

int get_size(void* current){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return -1;
	}

	int *header = (int *) current;
	int size = (*header) & 0xfc; //mask 0b11111100 -> because alignement min is 4 so the last 2 bits are gonna be 0 
	if (! (size & 0x7)){	
		fprintf(stderr, "Error : size not a multiple of 8");
	}
	return size;
}

int get_tag(void* current){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return -1;
	}

	int *header = (int *) current;
	int tag = (*header) & 1;
	return tag;
}

void set_next(void* current, void* next){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return;
	}

	if(!get_tag(current)){
		fprintf(stderr, "Error : current not free block");
		return;
	}

	if(!get_tag(next)){
		fprintf(stderr, "Error : next not free block");
		return;
	}

	int *header = (int *) current;

	if (next == NULL){ // case were next is null
		header[1] =  0; 
		return;
	}

	header[1] = (int) next;
}

void set_previous(void* current, void* previous){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return;
	}

	if(!get_tag(current)){
		fprintf(stderr, "Error : current not free block");
		return;
	}

	if(!get_tag(previous)){
		fprintf(stderr, "Error : previous not free block");
		return;
	}
	int *header = (int *) current;

	if (previous == NULL){ // case were previous is null
		header[2] =  0; 
		return;
	}
	
	header[2] = (int) previous; // intptr_t
}

void set_size(void* current, int size){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return;
	}

	int *header = (int *) current;
	int tag = get_tag(&current);
	if (! (size & 0x7)){	
		fprintf(stderr, "Error : size not a multiple of 8");
		return;
	}
	header[0] = size | tag;

	//*header = ((((*header) & 0) | size) << 1) | tag;
}

void set_tag(void* current, int tag){

	if (current == NULL){
		fprintf(stderr, "Error : null pointer");
		return;
	}

	int *header = (int *) current;
	*header = (*header & 0xfc) | tag; //mask 0b11111100 -> because alignement min is 4 so the last 2 bits are gonna be 0 

	//*header = (((*header) >> 1) << 1) | tag;
}

void remove_link(void* ptr){

	if(ptr == NULL){
		fprintf(stderr, "Error : null pointer");
		return;
	}

	void * next = get_next(ptr);
	void * previous = get_previous(ptr);

	if(previous == NULL){
		fprintf(stderr, "Error : pointer not in linked list");
		return;
	}

	set_next(previous, next);

	if(next != NULL){
		set_previous(next, previous);
	}
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // maybe initialize a global variable linked list ? maybe two for allocated and free ?
	start = NULL;
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

	size = size+16; // +16 for footer and header
	int newsize = ALIGN(size + SIZE_T_SIZE); 
	// search spot in heap to fit new block
	// Search appropriate free list for block of size m > n
	
	fprintf(stderr, "Searching smallest...\n");

	void* block = find_smallest_fit(start, newsize);
	fprintf(stderr, "'...smallest found'\n");

	if (block == NULL){

		fprintf(stderr, "No block found\n");
		block = mem_sbrk(newsize);

		if(block == (void*) -1){
			fprintf(stderr, "Error : sbrk allocation failed\n");
			return NULL;
		}

	}
	else{
		fprintf(stderr, "Block found\n");
		remove_link(block);
	}

	set_size(block, newsize);
	set_tag(block, 1);
	
	fprintf(stderr, "Returning\n");
	return block;
	

	// if spot not found, place the block at the end, allocating
	// more memory to the heap if needed

//  Below is the basic implementation given in template file
/*
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/
}

/*explicit free
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	if (ptr == NULL){
		fprintf(stderr, "Error : freeing a null ptr ");
		return;
	}	

	if(get_tag(ptr)){
		fprintf(stderr, "freeing an already freed ptr");
		return;
	}

	// no distinction needed between start already not NULL and start NULL

	set_tag(ptr, 0);
	set_next(ptr, start);
	set_previous(start, ptr);
	start = ptr;
	
	
	//~check that the pointer really is one
	/*
	if (ptr != NULL){

		double *tag = (double*)ptr; // this is to get the full tag
		unsigned char *allocated = ((unsigned char *)firstelt)+7; // last byte of the word tag

		//~check if the pointer has been returned by a malloc or realloc before (that is is it allocated ?)
		if ( *allocated == 1){

			*allocated = 0; // modify Boundary tag (allocated part)

			// add to appropriate linked list
		}
	}
	*/
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
    
	if(get_tag(oldptr)){
		fprintf(stderr, "Error : freeing already freed block");
		return NULL;
	}

    newptr = mm_malloc(size);

    if (newptr == NULL){
		fprintf(stderr, "Error : malloc failed");
		return NULL;
	}

    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize){
      copySize = size;
	}

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}



/*
 * mm_check - checks heap consistency
 */
int mm_checks(void)
{
	return 0;
}
