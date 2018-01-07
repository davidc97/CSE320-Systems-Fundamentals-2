/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include "sfmm2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};

int sf_errno = 0;

static void *find_fit(size_t asize);
static void place(sf_free_header *hp, size_t asize, size_t size);
static int find_list_index_from_size(size_t sz);
static void *new_page();
static void coalesce(void *hp);

void *sf_malloc(size_t size) {
    //first check if size is valid (<= 0 or greater than 4 pages(16384 bytes)
    if(size <= 0 || size > 16384){
        sf_errno = EINVAL;
        return NULL;
    }
    //Adjusted block size after adding header/footer/padding
    size_t asize;
    //pointer to header of allocated block(if found)
    void *hp;
    if(size < ALIGNMENT){
        asize = MINSIZE;
    }
    else{
        //add the overhead to the requested size then round up to nearest alignment
        asize = ALIGNMENT * ((size + ALIGNMENT + (ALIGNMENT-1))/ ALIGNMENT);
    }
    //search the segmented free list for a block that fits
    if((hp = find_fit(asize)) != NULL){
        place(hp, asize, size);
        //return pointer to payload section of allocated block
        return (hp + MEMROW);
    }
    //could not find fit, allocate new page
    for(int i = 0; i<(((asize/(PAGE_SZ))+1)); i++){
    hp = new_page();
    }
    if(hp != (void *) -1){
        place((sf_free_header *)hp, asize, size);
        return (hp + MEMROW);
    }
    sf_errno = ENOMEM;
	return NULL;
}
/*allocates memory for new page
 *returns pointer to beginning of new free block, or -1 if unable to allocate
 */
static void *new_page(){
    void *np = sf_sbrk();
    if(np != (void *) -1){
        sf_footer *prevf = (sf_footer *)(np - MEMROW);
        if((void *)prevf > get_heap_start()){
            if(prevf->allocated == 0){
                //coalesce backwards
                size_t asize = PAGE_SZ + (prevf->block_size << 4);
                void *tempf = (void *)prevf;
                sf_free_header *prevh = (sf_free_header *)((tempf - (prevf->block_size<<4)) + MEMROW);
                //remove previous free block from freelist
                int listnum = find_list_index_from_size(prevh->header.block_size<<4);
                free_list *list = &(seg_free_list[listnum]);
                    sf_free_header **ptr = &(list->head);
                     while(*ptr != NULL){
                     if(*ptr == prevh){
                            *ptr = NULL;
                      break;
                      }
                     ptr=&((*ptr)->next);
                     }
                prevh->prev = NULL;
                if(list->head == NULL){
                    list->head = prevh;
                    prevh->next = NULL;
                }
                else{
                    prevh->next = list->head;
                    list->head->prev = prevh;
                    list->head = prevh;
                }
                prevh->header.allocated = 0;
                prevh->header.padded = 0;
                prevh->header.two_zeroes = 00;
                prevh->header.block_size = asize >> 4;
                prevh->header.unused = 0;
                void *temp = (void *)prevh;
                sf_footer *ftr = (sf_footer *)((temp + asize) - MEMROW);
                ftr->allocated = 0;
                ftr->padded = 0;
                ftr->two_zeroes = 00;
                ftr->block_size = asize >> 4;
                ftr->requested_size =0;
                return prevh;
            }
        }
    sf_free_header *prevh = (sf_free_header *)(np);
        prevh->prev = NULL;
            int listnum = find_list_index_from_size(PAGE_SZ);
                free_list *list = &(seg_free_list[listnum]);
            if(list->head == NULL){
                list->head = prevh;
                prevh->next = NULL;
            }
            else{
                prevh->next = list->head;
                list->head->prev = prevh;
                list->head = prevh;
            }
            prevh->header.allocated = 0;
            prevh->header.padded = 0;
            prevh->header.two_zeroes = 00;
            prevh->header.block_size = PAGE_SZ >> 4;
            prevh->header.unused = 0;
            void *temp = (void *)prevh;
            sf_footer *ftr = (sf_footer *)((temp + PAGE_SZ) - MEMROW);
            ftr->allocated = 0;
            ftr->padded = 0;
            ftr->two_zeroes = 00;
            ftr->block_size = PAGE_SZ >> 4;
            ftr->requested_size =0;
            return prevh;
}
    return np;
}

static int find_list_index_from_size(size_t sz) {
    if (sz >= LIST_1_MIN && sz <= LIST_1_MAX) return 0;
    else if (sz >= LIST_2_MIN && sz <= LIST_2_MAX) return 1;
    else if (sz >= LIST_3_MIN && sz <= LIST_3_MAX) return 2;
    else return 3;
}
/*searches through the segregated free list looking for a block
 *asize = adjusted size of block including header/footer/padding
 *returns ptr to the header of the free block if found, NULL if no block found
 */
static void *find_fit(size_t asize){
    //index of which list to begin searching in
    int listnum;
    if(asize <= LIST_1_MAX){
        listnum = 0;
    }
    else if(asize <= LIST_2_MAX){
        listnum = 1;
    }
    else if(asize <= LIST_3_MAX){
        listnum = 2;
    }
    else{
        listnum = 3;
    }
    while(listnum < 4){
        sf_free_header *ptr = seg_free_list[listnum].head;
        while(ptr != NULL){
            if((ptr->header.block_size << 4) >= asize){
                //bp = &(ptr->header) + MEMROW;
                return ptr;
            }
            ptr=ptr->next;
        }
        listnum++;
    }
    //no fit
    return NULL;
}

static void place(sf_free_header *hp, size_t asize, size_t size){
    size_t bsize = (hp->header.block_size << 4);
    //remove free block from freelist
        int listnum = find_list_index_from_size(hp->header.block_size<<4);
        free_list *list = &(seg_free_list[listnum]);
        sf_free_header **ptr = &(list->head);
        while(*ptr != NULL){
            if(*ptr == hp){
                *ptr = NULL;
                break;
            }
            ptr=&((*ptr)->next);
        }
    //check if the leftover block is > 32 bytes. split if so
    if((bsize - asize) >= MINSIZE){
        //points *prev of next free block to previous block and *next of prev free block to next block
        if(hp->next != NULL){
            hp->next->prev = hp->prev;
        }
        if(hp->prev != NULL){
            hp->prev->next = hp->next;
        }
        hp->next = NULL;
        hp->prev = NULL;
        //adjusts header of newly allocated block
        //sf_header *headerptr = (sf_header *)hp;
        hp->header.block_size = (asize >> 4);
        hp->header.allocated = 1;
        if((size + ALIGNMENT) != asize){
            hp->header.padded = 1;
        }else{
            hp->header.padded = 0;
    }
        hp->header.two_zeroes = 0;
        hp->header.unused = 0;
        //fptr points to the footer of the newly allocated block
        void *temp = (void *)hp;
        sf_footer *fptr = (sf_footer *)((temp + asize) - MEMROW);
        fptr->block_size = (asize>>4);
        fptr->allocated = 1;
        fptr->padded = hp->header.padded;
        fptr->two_zeroes = 0;
        fptr->requested_size = size;
        //hptr points to the header of the block to split
        void *htemp = (void *)fptr;
        sf_header *hptr = (sf_header *)(htemp + MEMROW);
        hptr->block_size = ((bsize - asize)>>4);
        hptr->allocated = 0;
        hptr->padded = 0;
        hptr->two_zeroes = 0;
        hptr->unused = 0;
        //footptr points to the footer of the block to split
        void *temp2 = (void *)hptr;
        sf_footer *footptr = (sf_footer *)((temp2 + (bsize - asize) - MEMROW));
        footptr->block_size = ((bsize - asize)>>4);
        footptr->allocated = 0;
        footptr->padded = 0;
        footptr->two_zeroes = 0;
        footptr->requested_size = 0;
        //freeptr points to the free header of the newly split block
        sf_free_header *freeptr = (sf_free_header *) hptr;
        freeptr->header = *hptr;
        freeptr->prev = NULL;

        //inserts new free block into appropriate free list
        listnum = find_list_index_from_size(bsize-asize);
        //list = pointer to valid free list
        list = &(seg_free_list[listnum]);
        if(list->head == NULL){
            list->head = freeptr;
        }
        else{
        freeptr->next = list->head;
        list->head->prev = freeptr;
        list->head = freeptr;
        }
    }
    //if leftover block is < 32 bytes, it is a splinter so we don't split
    else{
        hp->header.allocated = 1;
        if((size + ALIGNMENT) != asize){
            hp->header.padded = 1;
        }else{
            hp->header.padded = 0;
    }
        hp->header.two_zeroes = 0;
        hp->header.unused = 0;
        if(hp->next != NULL){
            hp->next->prev = hp->prev;
        }
        if(hp-> prev != NULL){
            hp->prev->next = hp->next;
        }
        hp->next = NULL;
        hp->prev = NULL;
        void *temp = (void *)hp;
        sf_footer *fptr = (sf_footer *)((temp + bsize) - MEMROW);
        fptr->block_size = (bsize>>4);
        fptr->allocated = 1;
        fptr-> padded = hp->header.padded;
        fptr->requested_size = size;
        hp->header.two_zeroes = 0;
    }
}

void *sf_realloc(void *ptr, size_t size) {
    if(ptr == NULL){
        sf_errno = ENOMEM;
        abort();
    }
    sf_header *hptr = (sf_header *)(ptr-MEMROW);
    if((void *)hptr < get_heap_start() || (void *)hptr > get_heap_end()){
        sf_errno = ENOMEM;
        abort();
    }
    if(hptr->allocated != 1){
        sf_errno = ENOMEM;
        abort();
    }
    void *temp = (void *)hptr;
    sf_footer *fptr = (sf_footer *)((temp + (hptr->block_size<<4)) - MEMROW);
    if((hptr->allocated) != (fptr->allocated) || (hptr->block_size) != (fptr->block_size) ||
        (hptr->padded) != (fptr->padded)){
        sf_errno = ENOMEM;
        abort();
    }
    if((fptr->requested_size + ALIGNMENT) != ((fptr->block_size)<<4) && fptr->padded != 1){
        sf_errno = ENOMEM;
        abort();
    }
    if((fptr->requested_size + ALIGNMENT) == ((fptr->block_size)<<4) && fptr->padded != 0){
        sf_errno = ENOMEM;
        abort();
    }
    if(size < 0){
        sf_errno = ENOMEM;
        abort();
    }
    if(size == 0){
        sf_free(ptr);
        return NULL;
    }
    //reallocating to a larger size
    if(size > (fptr->requested_size)){
        void *new_block;
        if((new_block = sf_malloc(size)) != NULL){
        memcpy(new_block, ptr, (fptr->requested_size));
        sf_free(ptr);
        return new_block;
        }
    }
    //reallocating to a smaller size
    if(size < (fptr->requested_size)){
        size_t asize;
        if(size < ALIGNMENT){
        asize = MINSIZE;
         }
        else{
        //add the overhead to the requested size then round up to nearest alignment
        asize = ALIGNMENT * ((size + ALIGNMENT + (ALIGNMENT-1))/ ALIGNMENT);
        }
        if(((fptr->block_size)<<4) - asize > MINSIZE){
            //split if no splinter made
            void *temp = (void *)hptr;
            //address of newly split header
            sf_header *new_header = (sf_header *)(temp + asize);
            new_header->allocated=0;
            new_header->two_zeroes=0;
            new_header->padded=0;
            new_header->block_size = ((hptr->block_size) - (asize>>4));
            new_header->unused = 0;
            //address of newly split footer
            sf_footer *new_footer = (sf_footer *)((temp + asize +((new_header->block_size)<<4)) - MEMROW);
            new_footer->allocated=0;
            new_footer->two_zeroes=0;
            new_footer->padded=0;
            new_footer->block_size = new_header->block_size;
            new_footer->requested_size =0;
            //insert newly freed block into freelist
            sf_free_header *fheader = (sf_free_header *)new_header;
            fheader->header = *new_header;
            fheader->prev = NULL;
            hptr->block_size = asize>>4;
            coalesce((void *) fheader);
        }
        if(size + ALIGNMENT != (hptr->block_size)<<4){
                hptr->padded = 1;
            }
            else {
                hptr->padded = 0;
            }
        //if splinter, don't split
        void *temp = (void *)hptr;
        sf_footer *new_fptr = (sf_footer *)((temp + ((hptr->block_size)<<4)) - MEMROW);
        new_fptr->block_size = hptr->block_size;
        new_fptr->allocated = 1;
        new_fptr->two_zeroes = 0;
        new_fptr->requested_size = size;
        new_fptr->padded = hptr->padded;
        return ptr;
    }
	return NULL;
}

void sf_free(void *ptr) {
    if(ptr == NULL){
        abort();
    }
    sf_header *hptr = (sf_header *)(ptr-MEMROW);
    if((void *)hptr < get_heap_start() || (void *)hptr > get_heap_end()){
        abort();
    }
    if(hptr->allocated != 1){
        abort();
    }
    void *temp = (void *)hptr;
    sf_footer *fptr = (sf_footer *)((temp + (hptr->block_size<<4)) - MEMROW);
    if((hptr->allocated) != (fptr->allocated) || (hptr->block_size) != (fptr->block_size) ||
        (hptr->padded) != (fptr->padded)){
        abort();
    }
    if((fptr->requested_size + ALIGNMENT) != ((fptr->block_size)<<4) && fptr->padded != 1){
        abort();
    }
    if((fptr->requested_size + ALIGNMENT) == ((fptr->block_size)<<4) && fptr->padded != 0){
        abort();
    }

    hptr->allocated = 0;
    hptr->padded = 0;
    hptr->two_zeroes = 0;
    hptr->unused = 0;

    fptr->allocated = 0;
    fptr->padded = 0;
    fptr->two_zeroes = 0;
    fptr->requested_size = 0;

    coalesce(ptr-MEMROW);

	return;
}
/*coalesce hp with block of higher memory address if free
* inserts newly freed block in corresponding free list
*
**/
static void coalesce(void *hp){
    sf_free_header *free = (sf_free_header *)hp;
    sf_free_header *nextblock = (sf_free_header *)(hp + (free->header.block_size<<4));
    free->prev = NULL;
    int listnum = find_list_index_from_size(free->header.block_size<<4);
    free_list *list = &(seg_free_list[listnum]);
    //if nextblock is free, coalesce with hp
    if((nextblock->header.allocated) == 0){
        //remove nextblock from its freelist
        listnum = find_list_index_from_size(nextblock->header.block_size<<4);
        list = &(seg_free_list[listnum]);
        sf_free_header **ptr = &(list->head);
        while(*ptr != NULL){
            if(*ptr == nextblock){
                *ptr = NULL;
                break;
            }
            ptr=&((*ptr)->next);
        }
        if(nextblock->prev != NULL){
            nextblock->prev->next = nextblock->next;
        }
        if(nextblock->next != NULL){
            nextblock->next->prev = nextblock->prev;
        }
        nextblock->prev = NULL;
        nextblock->next = NULL;
        free->header.block_size += nextblock->header.block_size;
        //update footer of next block to be footer of coalesced block
        void * temp = (void *)nextblock;
        sf_footer *nextftr = (sf_footer *)(temp + ((nextblock->header.block_size<<4)) - MEMROW);
        nextftr->block_size = free->header.block_size;
    }
    //insert newly freed block at head of correct free list
    listnum = find_list_index_from_size(free->header.block_size<<4);
    list = &(seg_free_list[listnum]);
    if(list->head == NULL){
        list->head = free;
        free->next = NULL;
    }
    else{
        free->next = list->head;
        list->head->prev = free;
        list->head = free;
    }
}
