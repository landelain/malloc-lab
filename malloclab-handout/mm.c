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
//#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))


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

// global variables

static void* start;
static int size4 = sizeof(size_t) == 4;



// our auxiallary functions

void* find_smallest_fit(void* first, size_t size);
void* get_next(void* current);
void* get_previous(void* current);
size_t get_size(void* current);
unsigned char get_tag(void* current);
void set_next(void* current, void* next);
void set_previous(void* current, void* previous);
void set_size(void* current, size_t size);
void set_tag(void* current, unsigned char tag);

void remove_link(void* ptr);
int merge_link(void* ptr);
int create_link(void* ptr, size_t size_new);



void* find_smallest_fit(void* first, size_t size){
	void* current = first;
	void* best = NULL;
	int i = 0;
	//fprintf(stderr, "Starting loop\n");
	while(current != NULL){
		++i;
		if (i == 10){ // ig that some sort of first instance checking counter ?
			break;
		}
		if (get_size(current) == size){							// If exact size
			//fprintf(stderr, "Found exact size\n");
			return current;
		} else if (get_size(current) > size){						// If size ok
			//fprintf(stderr, "Found ok size\n");
			if (best == NULL){										// Set best if Null
				//fprintf(stderr, "Initialized best\n");
				best = current;
			} else if (get_size(best) > get_size(current)){		// Update best if better
				//fprintf(stderr, "Updated best\n");
				best = current;
			}
		}
		//fprintf(stderr, "Checking next\n");
		//fprintf(stderr, "Calling get next 2\n");
		current = get_next(current);
	}
	return best;													// Return best fit
}

void* get_next(void* current){

	if (current == NULL){
		fprintf(stderr, "Error get_next : null pointer\n");
		return NULL;
	}

	if(get_tag(current)){
		fprintf(stderr, "Error get_next : not free block\n");
		return NULL;
	}

	unsigned char *header = (unsigned char *) current;
	if (((void *) (header + ALIGNMENT)) == NULL){ // case were it doesnt have a next
		return NULL;
	}
	return (void *) (header + ALIGNMENT);
}

void* get_previous(void* current){

	if (current == NULL){
		fprintf(stderr, "Error get_previous : null pointer\n");
		return NULL;
	}

	if(get_tag(current)){
		fprintf(stderr, "Error get_previous : not free block\n");
		return NULL;
	}

	unsigned char *header = (unsigned char *) current;
	if (((void *) (header + 2*ALIGNMENT)) == NULL){ // case were it doesnt have a previous
		return NULL;
	}
	return (void *) (header + 2*ALIGNMENT);
}

size_t get_size(void* current){

	if (current == NULL){
		fprintf(stderr, "Error get_size : null pointer\n");
		return 0;
	}

	size_t * header = (size_t *) current;
	size_t size = 0;
	for (int i = 0; i < sizeof(size_t)-1; i++) {
		// Shift the bytes and combine into size
		size |= (size_t)header[i] << (sizeof(size_t) * i);
	}
	if(!size4){

		size = size & (~(size_t)0xff);
	}

	if ( size & 0x7 ){	

		fprintf(stderr, "Error get_size : size %zu not a multiple of 8\n", size);
	}
	return size;
}

unsigned char get_tag(void* current){

	if (current == NULL){
		fprintf(stderr, "Error get_tag : null pointer\n");
		return -1;
	}

	unsigned char *header = (unsigned char *) current;
	unsigned char tag = header[ALIGNMENT-1];
	return tag & 1;
}

void set_next(void* current, void* next){

	if (current == NULL){
		fprintf(stderr, "Error set_next : null pointer\n");
		return;
	}

	if(get_tag(current)){
		fprintf(stderr, "Error set_next : current not free block\n");
		return;
	}

	if(get_tag(next)){
		fprintf(stderr, "Error set_next : next not free block\n");
		return;
	}

	unsigned char *header = (unsigned char *) current;
	int **place = (int **) (header + ALIGNMENT);
	*place =(int *) next;
}

void set_previous(void* current, void* previous){

	if (current == NULL){
		fprintf(stderr, "Error set_previous : null pointer\n");
		return;
	}

	if(get_tag(current)){
		fprintf(stderr, "Error set_previous : current not free block\n");
		return;
	}

	if(get_tag(previous)){
		fprintf(stderr, "Error set_previous : previous not free block\n");
		return;
	}
	unsigned char *header = (unsigned char *) current;
	int *place = (int *) (header +2*ALIGNMENT);
	int prev = (int) previous;
	*place = prev;
}

void set_size(void* current, size_t size){
	//fprintf(stderr, "Setting size to %zu\n", size);

	if (current == NULL){
		fprintf(stderr, "Error set_size : null pointer\n");
		return;
	}

	if( size == 0){
		fprintf(stderr, "Error set_size : 0 size\n");
		return;
	}

	unsigned char *header = (unsigned char *) current;
	if ((size & 0x7)){	
		fprintf(stderr, "Error set_size : size not a multiple of 8\n");
		return;
	}

	header[0] = size;
	//size_t gsize = get_size(current);
	//fprintf(stderr, "Setting size to %zu, should be %zu\n", header[0], size);

}

void set_tag(void* current, unsigned char tag){

	if (current == NULL){
		fprintf(stderr, "Error set_tag : null pointer\n");
		return;
	}

	unsigned char *header = (unsigned char *) current;
	header[ALIGNMENT-1] = (header[ALIGNMENT-1] & (~1)) | tag;

}

void remove_link(void* ptr){

	if(ptr == NULL){
		fprintf(stderr, "Error remove_link : null pointer\n");
		return;
	}

	if(get_tag(ptr)){
		fprintf(stderr, "Error remove_link : not free block\n");
		return;
	}
	//fprintf(stderr, "Calling get next 3\n");
	void *next = get_next(ptr);
	void *previous = get_previous(ptr);

	if(next != NULL){
		set_previous(next, previous);
	}
	if(previous != NULL){
		set_next(previous, next);
	}

	if(previous == NULL){
		start = next;
	}
	set_tag(ptr, (unsigned char) 1);
}

int merge_link(void* ptr){

	if(ptr == NULL){
		fprintf(stderr, "Error merge_link : null pointer\n");
		return 1;
	}
	if(get_tag(ptr)){
		fprintf(stderr, "Error merge_link : pointer not free\n");
		return 1;
	}

	void* current = start;
	void* pre = NULL;
	void* post = NULL;
	size_t size = get_size(ptr);

	while(current != NULL){

		size_t sizec = get_size(current);
		if(current + sizec == ptr){
			//fprintf(stderr, "found pre");
			pre = current;
		}
		if(ptr + size == current){
			//fprintf(stderr, "found post");
			post = current;
		}
		
		//fprintf(stderr, "Calling get next 1, current tag: %d\n", get_tag(current));
		current = get_next(current);
	}

	if(post != NULL){
		size_t size_post = get_size(post);
		//fprintf(stderr, "Calling set size 1\n");
		set_size(ptr, size+size_post + 8);
		remove_link(post);
	}

	if(pre != NULL){
		size_t size_pre = get_size(pre);
		//fprintf(stderr, "Calling set size 2\n");

		set_size(pre, size+size_pre+8);
		remove_link(ptr);
	}


	return 0;
}

int create_link(void* ptr, size_t size_new){

	if(ptr == NULL){
		fprintf(stderr, "Error create_link : null pointer\n");
		return 1;
	}
	size_t size = get_size(ptr);
	size_t size_link = size-size_new-8;

	if(size_link < 16){
		fprintf(stderr, "Error create_link : not enough size\n");
		return 1; 
	}

	unsigned char * new_link_ptr = ( (unsigned char *) ptr ) + size_new;
	void* link_ptr = (void*) new_link_ptr;
	//fprintf(stderr, "Calling set size 3\n");

	set_size(link_ptr, size_link);
	set_tag(link_ptr, 0);

	set_next(link_ptr, start);
	set_previous(start, link_ptr);
	start = link_ptr;

	return 0;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
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

	size = size+ALIGNMENT; // header
	size_t newsize = ALIGN(size + SIZE_T_SIZE); 
	// search spot in heap to fit new block
	// Search appropriate free list for block of size m > n
	//fprintf(stderr, "Searching smallest...\n");

	void* block = find_smallest_fit(start, newsize);
	//fprintf(stderr, "'...smallest found'\n");

	if (block == NULL){

		//fprintf(stderr, "No block found\n");
		block = mem_sbrk(newsize);

		if(block == (void*) -1){
			fprintf(stderr, "Error mm_malloc : sbrk allocation failed\n");
			return NULL;
		}
		//fprintf(stderr, "Calling set size 4\n");

		set_size(block, newsize);

	}
	else{
		//fprintf(stderr, "Block found\n");

		size_t size_old = get_size(block);
		if (size_old - newsize < 24){
			remove_link(block);
		}
		else{
			//create_link(block, newsize);
			remove_link(block);
		}
	
	}
	//fprintf(stderr, "Size: %d, new_size: %d\n", size, newsize);
	set_tag(block, 1);
	
	//fprintf(stderr, "Returning\n");

	unsigned char *ptr = (unsigned char *) block;
	ptr = ptr+ALIGNMENT;

	return (void *) ptr;
	

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
void mm_free(void *ptr){

	if (ptr == NULL){
		fprintf(stderr, "Error mm_free : freeing a null ptr\n");
		return;
	}	

	unsigned char *head = (unsigned char *) ptr;
	head = head-ALIGNMENT;
	ptr = (void *) head;

	if(!get_tag(ptr)){
		fprintf(stderr, "Error mm_free : freeing an already freed ptr\n");
		return;
	}

	// no distinction needed between start already not NULL and start NULL

	set_tag(ptr, (unsigned char)0);
	if (get_tag(ptr)){
		fprintf(stderr, "Unexpected tag\n");
	}
	set_next(ptr, start);
	set_previous(start, ptr);
	start = ptr;

	merge_link(ptr);
	
	
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
		fprintf(stderr, "Error mm_realloc : freeing already freed block\n");
		return NULL;
	}

    newptr = mm_malloc(size);

    if (newptr == NULL){
		fprintf(stderr, "Error mm_realloc : malloc failed\n");
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
