#include "vm/frame.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"
#include <hash.h>
#include "threads/vaddr.h"

struct hash frame_table;


/* init frame table, each entry would be inserted or replaced 
	when s_page_table entry added */
void 
frametable_init(void)
{	
	hash_init(&frame_table, frame_hash, frame_less, NULL);
}

/*
	void add_frame_entry(void *paddr, void *vaddr, tid_t pid)
	from s_pte_insert()

	1. for every new mapping with frame addr & page addr, 
		add new entry to frame table too.
	2. use hash_replace that insert the entry
		or replace(if there is) (remove the entry and insert new again).
*/

void
add_frame_entry(void *paddr, void *vaddr, tid_t pid)
{
	struct frame *f;
	f = malloc(sizeof *f);
	f->paddr = paddr;
	f->vaddr = vaddr;
	f->pid = pid;
	hash_replace(&frame_table, &f->hash_elem);
}


/* 
	find frame table entry with va, pid
	and then delete free entry 
*/
void
free_frame_entry(void *va, tid_t pid)
{
	struct frame* f;
	f = frame_lookup_vaddr(va, pid);
	hash_delete (&frame_table, &f->hash_elem);

	/*
	if (f != NULL){
		f->vaddr = NULL;
		f->pid = NULL;
	}
	*/
	

}	


/* Returns the frame containing the given physical address (entry), 
	or a null pointer if no such frame exists */
struct frame *
frame_lookup_paddr (void* paddr)
{
	struct frame *f;
	struct hash_elem *e;
	f = malloc(sizeof *f);
	f->paddr = paddr;
	e = hash_find (&frame_table, &f->hash_elem);
	return e != NULL ? hash_entry (e, struct frame, hash_elem) : NULL;
}


/* Returns the frame containing the given virtual address and pid, 
	or a null pointer if no such frame exists */
struct frame *
frame_lookup_vaddr(const void *vaddr, tid_t pid)
{
	struct hash_iterator i;

	hash_first (&i, &frame_table);
	while (hash_next (&i))
	{
		struct frame *f; 
		f = hash_entry (hash_cur (&i), struct frame, hash_elem);
		if (f->vaddr == vaddr && f->pid == pid){
			return f;
		}

	}
	return NULL;

}






/* Get free frame from frame table and return uint32_t paddr */
void *
get_free_frame(void)
{
	void *vaddr;
	vaddr = palloc_get_page(PAL_USER); 		//find a empty entry.

	//case 1. if there are free frame
	if(vaddr != NULL){
		struct frame* f;
		f = malloc(sizeof *f);
		f->paddr = vtop(vaddr);
		return f->paddr;
	}
	//case 2. there are no free frame -> eviction
	else{
		/*
		1. choose frame to evict (page table의 accessed, dirty bits)
		2. remove references to the frame from any page table
		3. write the page to the file system / swap
		*/
		struct frame *victim_frame; 
		uint32_t victim = (uint32_t) get_victim();

		victim_frame = frame_lookup_paddr((void *) victim);
		
		if(victim_frame ==NULL)
		{
			printf("error : no victim_frame\n");
			ASSERT(0);
		}

		swap_out(victim_frame->paddr, victim_frame->vaddr, victim_frame->pid);		//physical memory에 빈공간 만들어
		
		// 그 mapping 지우는 과정
		uint32_t *pd; 
		pd = find_process(victim_frame->pid)->thread->pagedir;
		pagedir_clear_page (pd, victim_frame->vaddr);


		return (void *) victim;
		//swap_in에서 victim_frame->vaddr로 넣어줌 
		
	}
	
}




/* Returns a hash value for frame f*/
unsigned
frame_hash (const struct hash_elem *f_, void *aux UNUSED)
{
	const struct frame *f = hash_entry (f_, struct frame, hash_elem);
	return hash_bytes (&f->paddr, sizeof f->paddr);
}

/* Returns true if frame a precedes page b */
bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
	const struct frame *a = hash_entry (a_, struct frame, hash_elem);
	const struct frame *b = hash_entry (b_, struct frame, hash_elem);

	return a->paddr < b->paddr;
}

