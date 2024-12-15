/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

//size in bytes
const size_t kBlockHeaderSize = 16;

#define fakeprintf(...)
#define fakeputs(...)
/* rounds up to the nearest multiple of ALIGNMENT */

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

uint8_t* current_avail = NULL;
int current_avail_size = 0;
int min_page_size = 0;
const int initial_page_count = 8;
typedef struct block_header
{
  //denotes the size of the whole block, including the header bytes that this struct occupies
  int blocksize;
  struct block_header*next;
}block_header;
block_header* firstGap = NULL;
void do_cycle_check()
{
	fakeprintf("doing cycle check!\n");
	int counter = 0;
	for (block_header* iterator = firstGap; iterator != NULL; iterator = iterator->next)
	{
		++counter;
		if (counter % 1000000 == 0)
		{
		  fakeprintf("counter: %d\n", counter);
		}
	}
	fakeprintf("done with cycle check! num items: %d\n", counter);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  fakeputs("mm_init() called!");
  // remember to handle use case when this is called multiple times during the run of mdriver
  
  // only do this is current avail is actually null
  mem_reset();
  mem_init();
  min_page_size = mem_pagesize();
  void *initialAllocation = mem_map(min_page_size * initial_page_count);
  if(initialAllocation == NULL)
  {
    return -1;
  }

  current_avail = NULL;
  current_avail_size = 0;
  firstGap = NULL;
  
  return 0;
}

uint8_t* getBeginningOfUserArea(const block_header* headerPtr)
{
  return (((uint8_t*)headerPtr) + kBlockHeaderSize);
}

  void *mallocphase2(const size_t requestedSize)
  {
  //add sizeof(int) so that we have room for the beginning of the block header before the user's data begins
  size_t newsize = ALIGN(requestedSize + kBlockHeaderSize);
  
  if (current_avail_size < newsize) {
    current_avail_size = PAGE_ALIGN(newsize);
    current_avail = mem_map(current_avail_size);
    if (current_avail == NULL)
      return NULL;
  }

  uint8_t *p = current_avail;
  block_header* temp = (block_header*)p;
  //*temp = newsize;
  //temp->blocksize = newsize;

  
  //this is the confusing part of the code where we can't write to the
  //temp->next member of `temp` because all the memory past the 4 bytes will
  //live within the user's area of the allocated block that they control. so
  //we just leave the `next` member as garbage since the user will overwrite it
  temp->blocksize = newsize;
  current_avail += newsize;
  current_avail_size -= newsize;
  // uint8_t* payload = (uint8_t*)((char*)p + sizeof(int));
  // uintptr_t alignedPayload = (uintptr_t)payload;
  // if(alignedPayload%ALIGNMENT != 0)
  // {
  //   alignedPayload += ALIGNMENT - (alignedPayload % ALIGNMENT);
  // }
  // if((alignedPayload + requestedSize) >= (uintptr_t)((char*)current_avail + current_avail_size))
  // {
  //   //allocate a new page
    
  // }
  
  return (void*)getBeginningOfUserArea(temp);
}

//implement this in case we want to break a block into a smaller occupied and unoccupied block for performance reasons
/*
void unMergeBlocks(const size_t requestedSize, block_header* destinationBlock)
{

}
*/

void *mallocphase1_internal(const size_t requestedSize)
{
  block_header* previousIteration = NULL;
  block_header* currentIterator = firstGap;
  int debug_counter = 0;
  while(currentIterator != NULL)
 {

   fakeprintf("current: %p, prev: %p\n", currentIterator, previousIteration);
    if (currentIterator == previousIteration)
    {
      fakeprintf("warning: symptom!\n");
      __builtin_trap();
    }
  const int localBlockSize = currentIterator->blocksize;
  //if this block is suitable for the requested allocation
    if ((localBlockSize - kBlockHeaderSize) >= requestedSize)
    {
      //if previousIteration is null, then that means we chose to allocate something in the first gap
      if (previousIteration == NULL)
      {
	fakeprintf("other thing was true\n");
        firstGap = currentIterator->next;
      }
      else
      {
	fakeprintf("one: %p\n", previousIteration);
	fakeprintf("two: %p\n", previousIteration->next);
	fakeprintf("three: %p\n", currentIterator);
	fakeprintf("four: %p\n", currentIterator->next);
	/*
        currentIterator->next = previousIteration->next;
	if (currentIterator == previousIteration)
	{
	  fakeprintf("warning: cycle!\n");
	}
        previousIteration->next = currentIterator;
	*/
	previousIteration->next = currentIterator->next;
      }
      /*
      //multiplying by 3 cause this is only worth doing if we have space for the
      //existing block header, a new block header, and another 16 bytes to actually allocate to the user
      if ((localBlockSize - (kBlockHeaderSize * 3)) >= requestedSize)
      {
        unmerge(requestedSize, currentIterator);
      }
      */
      return (void*)getBeginningOfUserArea(currentIterator);
    }

    ++debug_counter;
    previousIteration = currentIterator;
    currentIterator = currentIterator->next;
 }
  return NULL;

}

void* mallocphase1(const size_t requestedSize)
{
  fakeprintf("begin phase 1!\n");
  void* result = mallocphase1_internal(requestedSize);
  fakeprintf("end phase 1!\n");
  do_cycle_check();
  return result;
}


/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t requestedSize)
{
  do_cycle_check();
  void* returnValue = mallocphase1(requestedSize);
  if (returnValue != NULL)
  {
    fakeprintf("got something from phase 1! %p\n", returnValue);
  }
  if(returnValue == NULL)
  {
    returnValue = mallocphase2(requestedSize);
    if (returnValue != NULL)
    {
      fakeprintf("got something from phase 2! %p\n", returnValue);
    }
  }
  if (returnValue == ((uint8_t*)firstGap + kBlockHeaderSize))
  {
    fakeprintf("warning: returning firstGap, but we've implemented that so it's fine\n");
  }
  do_cycle_check();
  return returnValue;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free_internal(void *voidptr)
{
  fakeprintf("freeing: %p\n", voidptr);
  uint8_t* ptr = (uint8_t*)voidptr;
  block_header* ourBlock = (block_header*)(ptr - kBlockHeaderSize);
  //int blockSize = *temp;

  // first time anything has been freed since we have initialized:
 //insert block in free list and merge if required with another block:
 block_header* previousGap = firstGap;
 //block_header* previousIteration = NULL;
 /*
 while(previousGap != NULL && previousGap < temp)
 {
    // previousGap8bitPtr = (uint8_t*)previousGap;
    // previousGap8bitPtr += 4;
    // previousGap = (int**)previousGap8bitPtr;
    previousGap = previousGap->next;
    previousIteration = previousGap;
 }
 */

 //use case: emtpy linked list
 if (firstGap == NULL)
 {
   fakeputs("empty linked list!");
   firstGap = ourBlock;
   firstGap->next = NULL;
   return;
 }

 //use case: our new block has a lower memory address than firstGap
 if (ourBlock < firstGap)
 {
   fakeputs("lower than firstGap!");
   ourBlock->next = firstGap;
   firstGap = ourBlock;
   return;
 }

 block_header* gapPrecedingOurGap = NULL;
 for (block_header* blockIterator = firstGap; blockIterator != NULL; blockIterator = blockIterator->next)

 {
    // previousGap8bitPtr = (uint8_t*)previousGap;
    // previousGap8bitPtr += 4;
    // previousGap = (int**)previousGap8bitPtr;
	 /*
    previousGap = previousGap->next;
    previousIteration = previousGap;
    */
    if ((blockIterator->next > ourBlock) || (blockIterator->next == NULL))
    {
      gapPrecedingOurGap = blockIterator;
      break;
    }
 }

 ourBlock->next = gapPrecedingOurGap->next;
 gapPrecedingOurGap->next = ourBlock;

  //disabling merge code for now!
  const int kShouldDoMerging = 0;


 // merge if required:
  if (kShouldDoMerging == 1)
  {
    block_header *nextBlock = (block_header*)(((char*)ourBlock)+(ourBlock->blocksize));
    if(nextBlock == previousGap)
    {
      ourBlock->blocksize += previousGap -> blocksize;
      // *(temp + 1) = *(int**) (*previousGap + 1);
    }
  }

  //*(int**) (temp +1) = *previousGap;
  //temp->blocksize = blocksize;

  // int* currentGap = firstGap;
  // int** nextGap = (int**) (currentGap + 1);
  
  // int** doubleNextGap = (int**)(*nextGap + 1);
  // int thisBlockIsBeforeOurBlock = (*doubleNextGap) < temp;
  // // assume that `doubleNextGap` _does_ point past the gap that we're currently freeing
  // // todo: turn this into a loop
  // while(nextGap != NULL)
  // {
  // if (thisBlockIsBeforeOurBlock)
  // {
  //   int* previousDoubleNextGap = *doubleNextGap;
  //   //make the `next` pointer in nextGap's memory area refer to our new gap, since our
  //   //new gap is now the closest gap to nextGap
  //   *doubleNextGap = temp;
  //   int** gapAfterTemp = (int**)(temp + 1); // it would be useful to make a helper function to convert
  //   //a pointer to a gap into a double pointer to the `next` pointer that's embedded in our gap
  //   *gapAfterTemp = previousDoubleNextGap;
  // }
  // }
  //handle case where we only have one item:
}

void mm_free(void *voidptr)
{
  do_cycle_check();
  fakeprintf("begin free!\n");
  if (voidptr == 0x7f2ceb2c1010)
  {
    fakeprintf("blah!\n");
  }
	mm_free_internal(voidptr);
  fakeprintf("end free!\n");
  do_cycle_check();
}
