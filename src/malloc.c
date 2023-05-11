#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */

void count_blocks();

void printStatistics( void )
{
   //num_blocks
   count_blocks();

   printf("\nheap management statistics\n");
   printf("mallocs:\t%d\n", num_mallocs );
   printf("frees:\t\t%d\n", num_frees );
   printf("reuses:\t\t%d\n", num_reuses );
   printf("grows:\t\t%d\n", num_grows );
   printf("splits:\t\t%d\n", num_splits );
   printf("coalesces:\t%d\n", num_coalesces );
   printf("blocks:\t\t%d\n", num_blocks );
   printf("requested:\t%d\n", num_requested );
   printf("max heap:\t%d\n", max_heap );
}
//24 bytes + 4 bytes = 28 bytes min
struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                            */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3ByaW5nIDIwMjM            */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *NF = NULL;
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */

void count_blocks() 
{
   struct _block* current = heapList;

   //Iterates through the list and counts the total blocks
   while (current != NULL) 
   {
      num_blocks++;
      current = current->next;
   }
}

struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }

#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   //Create a Best fit block 
   struct _block *BF = NULL;

   //Used for computing and finding the block with the least remainder 
   int max = INT_MAX;

   //While curr is not NULL
   while (curr)
   {
      //If free, size is big enough, and Best fit block is NULL or size is less than max
      if (curr->free && curr->size >= size && (!BF || (int)curr->size < max))
      {
         //Best fit is now curr
         //and max is set to the size found
         BF = curr;
         max = (int)curr->size;
      }

      *last = curr;
      curr = curr->next;
   }

   return BF;

   /** \TODO Implement best fit here */
#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   //Create a Worst fit block
   struct _block *WF = NULL;

   //Used for computing and finding the block with the most remainder 
   int min = INT_MIN;

   //While curr is not NULL
   while (curr)
   {
      //If free, size is big enough, and Worst fit block is NULL or size is greater than max
      if (curr->free && curr->size >= size && (!WF || (int)curr->size > min))
      {
         WF = curr;
         min = (int)curr->size;
      }

      *last = curr;
      curr = curr->next;
   }

   return WF;

   /** \TODO Implement worst fit here */
#endif

// \TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0

   //Start from the next block after the previous free block found
   if (NF) 
   {
      curr = NF->next;
   }

   //Iterate through the list of blocks and
   //find a free block with enough space
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr = curr->next;
   }

   //Start from the beginning of the list
   //if no free blocks were found from the NF block onward
   if (!curr) 
   {
      curr = heapList;

      while (curr && !(curr->free && curr->size >= size)) 
      {
         *last = curr;
         curr = curr->next;
      }
   }

   //Update the NF block if a free block was found
   else if (curr) 
   {
      NF = curr;
   }

   return NF;
   /** \TODO Implement next fit here */
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   max_heap += (int) size;
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;

   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{ 
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;

      struct _block* head = heapList;

      while (head != NULL) 
      {
         num_blocks++;
         head = head->next;
      }


      atexit( printStatistics );
   }

   num_requested += size;

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */
  
   if (next != NULL && next->size > size + sizeof(struct _block) + 4)
   {
      //Calculate left over size from the block found
      size_t leftover = next->size - size + sizeof(struct _block);
      
      //Calculate the location in memory of the new block
      struct _block *new_block = (struct _block*) ((char*) next + size + sizeof(struct _block));

      //Set the members of the new block
      new_block->size = leftover - sizeof(struct _block);
      new_block->free = true;
      new_block->next = next->next;

      //Adjust the members in the current block
      next->size = size;
      next->next = new_block;

      num_splits++;
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);

      num_grows++;
   }

   else
   {
      num_reuses++;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;

   num_mallocs++;
   
   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
         are free then combine them with this block being freed.
   */

   /* Coalesce free _blocks */
   //prev is the top of the list
   struct _block *prev = heapList;

   //next connected to curr in the list
   struct _block *next = curr->next;

   // Coalesce with the next block if it's free
   if (next && next->free)
   {
      curr->size += sizeof(struct _block) + next->size;
      curr->next = next->next;

      num_coalesces++;

      if (NF && NF->free)
      {
         NF = curr;
      }
   }

   // Coalesce with the previous block if it's free
   while (prev && prev->next != curr)
   {
      prev = prev->next;
   }

   if (prev && prev->free)
   {
      prev->size += sizeof(struct _block) + curr->size;
      prev->next = curr->next;

      num_coalesces++;

      if (NF && NF->free)
      {
         NF = curr;
      }
   }

   num_frees++;
}

void *calloc( size_t nmemb, size_t size )
{
   //Initialize a ptr with a malloc call
   void *ptr = malloc(nmemb * size);

   //If ptr is not empty set the memory space to 0
   if (ptr)
   {
      memset(ptr, 0, nmemb * size);

      return ptr;
   }
   // \TODO Implement calloc
   return NULL;
}

void *realloc( void *ptr, size_t size )
{
   //If ptr passed in is not NULL return the malloc call with the new size
   if (!ptr)
   {
      return malloc(size);
   }

   //If size passed in is 0 then free ptr
   else if (size == 0)
   {
      free(ptr);
   }

   //If pointer is NULL, create a new ptr with the new size allocated 
   else 
   {
      void *new_ptr = malloc(size);

      //If new_ptr is not NULL, call memcpy to set the memory block to 0
      //and free the old pointer (ptr)
      if (new_ptr)
      {
         memcpy(new_ptr, ptr, size);

         free(ptr);
      }

      return new_ptr;
   }
   // \TODO Implement realloc
   return NULL;
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwMjM= -----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
