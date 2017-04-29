#include <stdio.h>
#include <hash.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "userprog/process.h"

struct frame {
	struct hash_elem hash_elem;			/* Hash table element */
	void *paddr;						/* (key of hash) physical address */
	void *vaddr;						/* points virtual address page */
	tid_t pid;							/* process's pid that allocate this frame */		
};


struct hash frame_table;






void frametable_init(void);
void add_frame_entry(void *, void *, tid_t);
void free_frame_entry(void *, tid_t);
struct frame * frame_lookup_paddr(void*);
struct frame * frame_lookup_vaddr(const void *, tid_t);
void * get_free_frame(void);
unsigned frame_hash (const struct hash_elem *, void * UNUSED);
bool frame_less (const struct hash_elem *, const struct hash_elem *, void * UNUSED);

