#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "sfmm.h"


int find_list_index_from_size(int sz) {
	if (sz >= LIST_1_MIN && sz <= LIST_1_MAX) return 0;
	else if (sz >= LIST_2_MIN && sz <= LIST_2_MAX) return 1;
	else if (sz >= LIST_3_MIN && sz <= LIST_3_MAX) return 2;
	else return 3;
}

Test(sf_memsuite_student, Malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x);

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	sf_header *header = (sf_header*)((char*)x - 8);

	/* There should be one block of size 4064 in list 3 */
	free_list *fl = &seg_free_list[find_list_index_from_size(PAGE_SZ - (header->block_size << 4))];

	cr_assert_not_null(fl, "Free list is null");

	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert_null(fl->head->next, "Found more blocks than expected!");
	cr_assert(fl->head->header.block_size << 4 == 4064);
	cr_assert(fl->head->header.allocated == 0);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(get_heap_start() + PAGE_SZ == get_heap_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, Malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 2);

	cr_assert_null(x, "x is not NULL!");
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_double_free, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));
	sf_free(x);
	sf_free(x);
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(sizeof(long));
	void *y = sf_malloc(sizeof(double) * 10);
	/* void *z = */ sf_malloc(sizeof(char));

	sf_free(y);

	free_list *fl = &seg_free_list[find_list_index_from_size(96)];

	cr_assert_not_null(fl->head, "No block in expected free list");
	cr_assert_null(fl->head->next, "Found more blocks than expected!");
	cr_assert(fl->head->header.block_size << 4 == 96);
	cr_assert(fl->head->header.allocated == 0);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(sizeof(long));
	void *x = sf_malloc(sizeof(double) * 11);
	void *y = sf_malloc(sizeof(char));
	/* void *z = */ sf_malloc(sizeof(int));

	sf_free(y);
	sf_free(x);

	free_list *fl_y = &seg_free_list[find_list_index_from_size(32)];
	free_list *fl_x = &seg_free_list[find_list_index_from_size(144)];

	cr_assert_null(fl_y->head, "Unexpected block in list!");
	cr_assert_not_null(fl_x->head, "No block in expected free list");
	cr_assert_null(fl_x->head->next, "Found more blocks than expected!");
	cr_assert(fl_x->head->header.block_size << 4 == 144);
	cr_assert(fl_x->head->header.allocated == 0);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	/* void *u = */ sf_malloc(1);          //32
	void *v = sf_malloc(LIST_1_MIN); //48
	void *w = sf_malloc(LIST_2_MIN); //160
	void *x = sf_malloc(LIST_3_MIN); //544
	void *y = sf_malloc(LIST_4_MIN); //2080
	/* void *z = */ sf_malloc(1); // 32

	int allocated_block_size[4] = {48, 160, 544, 2080};

	sf_free(v);
	sf_free(w);
	sf_free(x);
	sf_free(y);

	// First block in each list should be the most recently freed block
	for (int i = 0; i < FREE_LIST_COUNT; i++) {
		sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
		cr_assert_not_null(fh, "list %d is NULL!", i);
		cr_assert(fh->header.block_size << 4 == allocated_block_size[i], "Unexpected free block size!");
		cr_assert(fh->header.allocated == 0, "Allocated bit is set!");
	}

	// There should be one free block in each list, 2 blocks in list 3 of size 544 and 1232
	for (int i = 0; i < FREE_LIST_COUNT; i++) {
		sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
		if (i != 2)
		    cr_assert_null(fh->next, "More than 1 block in freelist [%d]!", i);
		else {
		    cr_assert_not_null(fh->next, "Less than 2 blocks in freelist [%d]!", i);
		    cr_assert_null(fh->next->next, "More than 2 blocks in freelist [%d]!", i);
		}
	}
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 10);

	free_list *fl = &seg_free_list[find_list_index_from_size(32)];

	cr_assert_not_null(x, "x is NULL!");
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.block_size << 4 == 32, "Free Block size not what was expected!");

	sf_header *header = (sf_header*)((char*)x - 8);
	cr_assert(header->block_size << 4 == 64, "Realloc'ed block size not what was expected!");
	cr_assert(header->allocated == 1, "Allocated bit is not set!");
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int) * 8);
	void *y = sf_realloc(x, sizeof(char));

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_header *header = (sf_header*)((char*)y - 8);
	cr_assert(header->allocated == 1, "Allocated bit is not set!");
	cr_assert(header->block_size << 4 == 48, "Block size not what was expected!");

	free_list *fl = &seg_free_list[find_list_index_from_size(4048)];

	// There should be only one free block of size 4048 in list 3
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.allocated == 0, "Allocated bit is set!");
	cr_assert(fl->head->header.block_size << 4 == 4048, "Free block size not what was expected!");
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_header *header = (sf_header*)((char*)y - 8);
	cr_assert(header->block_size << 4 == 32, "Realloc'ed block size not what was expected!");
	cr_assert(header->allocated == 1, "Allocated bit is not set!");


	// After realloc'ing x, we can return a block of size 48 to the freelist.
	// This block will coalesce with the block of size 4016.
	free_list *fl = &seg_free_list[find_list_index_from_size(4064)];

	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.allocated == 0, "Allocated bit is set!");
	cr_assert(fl->head->header.block_size << 4 == 4064, "Free block size not what was expected!");
}


//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

Test(sf_memsuite_student, realloc_size_zero, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(long));
	/*void *y */sf_realloc(x, 0);

	free_list *fl = &seg_free_list[find_list_index_from_size(PAGE_SZ)];
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.allocated == 0, "allocated bit is set!");
	cr_assert(fl->head->header.block_size << 4 == PAGE_SZ, "Free block size not what was expected!");

}

Test(sf_memsuite_student, check_padded_bit, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(16);
	sf_header *header = (sf_header*)((char*)x - 8);
	sf_footer *footer = (sf_footer*)((char*)x + 16);
	cr_assert(header->block_size << 4 == 32, "Block size incorrect");
	cr_assert(header->padded == 0, "Padded bit is set!");
	cr_assert(header->allocated == 1, "allocated bit is not set!");
	cr_assert(footer->block_size << 4 == 32, "block size incorrect");
	cr_assert(footer->padded == 0, "padded bit is set!");
	cr_assert(footer->allocated == 1, "allocated bit is not set!");
	cr_assert(footer->requested_size == 16, "requested_size is not what was expected");
	void *y = sf_realloc(x, 10);
	header = (sf_header*)((char*)y - 8);
	footer = (sf_footer*)((char*)y + 16);
	cr_assert(header->padded == 1, "Padded bit is not set!");
	cr_assert(footer->padded == 1, "Padded bit is not set!");
}

Test(sf_memsuite_student, full_malloc, .init = sf_mem_init, .fini = sf_mem_fini){
	void *x = sf_malloc(PAGE_SZ - 16);
	void *y = sf_malloc(PAGE_SZ - 16);
	void *w = sf_malloc(PAGE_SZ - 16);
	void *z = sf_malloc(PAGE_SZ - 16);

	sf_header *header = (sf_header*)((char*)x - 8);
	sf_header *header2 = (sf_header*)((char*)y - 8);
	sf_header *header3 = (sf_header*)((char*)w - 8);
	sf_header *header4 = (sf_header*)((char*)z - 8);

	cr_assert(header->block_size << 4 == PAGE_SZ, "Incorrect block size");

	cr_assert(header2->block_size << 4 == PAGE_SZ, "Incorrect block size");

	cr_assert(header3->block_size << 4 == PAGE_SZ, "Incorrect block size");

	cr_assert(header4->block_size << 4 == PAGE_SZ, "Incorrect block size");

	free_list *fl1 = &seg_free_list[0];
	free_list *fl2 = &seg_free_list[1];
	free_list *fl3 = &seg_free_list[2];
	free_list *fl4 = &seg_free_list[3];


	cr_assert_null(fl1->head, "Unexpected block in free list 1");
	cr_assert_null(fl2->head, "Unexpected block in free list 2");
	cr_assert_null(fl3->head, "Unexpected block in free list 3");
	cr_assert_null(fl4->head, "Unexpected block in free list 4");
}

Test(sf_memsuite_student, malloc_invalid, .init = sf_mem_init, .fini = sf_mem_fini){
	sf_errno = 0;

	void *x = sf_malloc(0);

	cr_assert_null(x, "x is not NULL!");
	cr_assert(sf_errno == EINVAL, "sf_errno is not EINVAL!");
}

Test(sf_memsuite_student, double_realloc_same_block, .init = sf_mem_init, .fini = sf_mem_fini){

	void *x = sf_malloc(sizeof(double) * 10); //block size = 96

	sf_header *header = (sf_header*)((char *)x - 8);
	cr_assert(((header->block_size)<<4) == (96), "Wrong block size");

	sf_realloc(x, 64);
	//freelist should not have any block because splinter
	//This assumes that the initial free block from sf_sbrk is in a different free list, so this test may not pass with certain list sizes
	free_list *fl1 = &seg_free_list[find_list_index_from_size(32)];
	cr_assert(((header->block_size)<<4) == (96), "Wrong block size");
	cr_assert_null(fl1->head, "Unexpected block!");

	fl1 = &seg_free_list[find_list_index_from_size(4060)];
	sf_realloc(x, 2); //block size = 32
	cr_assert(fl1->head->header.block_size<<4 == 4064, "wrong free block size");
	cr_assert(((header->block_size)<<4) == (32), "Wrong block size");
}