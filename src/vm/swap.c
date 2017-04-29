#include "vm/swap.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "devices/disk.h"
#include <string.h>
#include "threads/vaddr.h"

static struct disk *disk;
static disk_sector_t sector;
struct hash swap_table;


void
swap_init(void){
	hash_init(&swap_table, swap_hash, swap_less, NULL);
	sector = 0;
	/* Get Swap Disk */
	disk = disk_get (1, 1);
	if (disk == NULL)
	    PANIC ("couldn't open target disk (swap or hd1:1)");
}

/* Returns a hash value for swap_entry s*/
unsigned
swap_hash (const struct hash_elem *s_, void *aux UNUSED)
{
	const struct swap_entry *s = hash_entry (s_, struct swap_entry, hash_elem);
	return hash_bytes (&s->disksector, sizeof s->disksector);
}

/* Returns true if frame a precedes page b */
bool
swap_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
	const struct swap_entry *a = hash_entry (a_, struct swap_entry, hash_elem);
	const struct swap_entry *b = hash_entry (b_, struct swap_entry, hash_elem);

	return a->disksector < b->disksector;
}



/* 
TRACKS IN-USE AND FREE SWAP SLOTS

1. "Picking an unused swap slot" for evicting a page from its frame to the swap partition
2. "Freeing a swap slot" when its page is read back into frame / page가 swapped 된 process가 terminate

*/




/* page fault시에 disk에서 불러옴 */
void
swap_in(void *paddr, void *vaddr, tid_t pid){
/* 
1. vaddr, pid을 가지고 있는 swap_entry 찾는다 
2. disk_read로 free_frame에 읽어온다
3. swap_table에서 swap_entry의 vaddr = NULL, pid = NULL
*/
	void *buffer;
	struct swap_entry *s;

	/* Find sector of vaddr */
	s = swap_lookup_vaddr(vaddr, pid);

	/* Allocate buffer. */
	buffer = malloc (DISK_SECTOR_SIZE);
	if (buffer == NULL)
		PANIC ("couldn't allocate buffer");

	/* Read from disk */
	disk_read (disk, s->disksector, buffer);

	/* Update swap table */
	s->vaddr = NULL;
	s->pid = (int)NULL;

	memcpy (paddr, buffer, DISK_SECTOR_SIZE);

}

/* frame에서 victim frame찾아서 swap out */
void
swap_out(void *paddr, void *vaddr, tid_t pid){
/* 
1. write 해줄 disk sector를 찾는다. (free인 sector)
2. disk_write로 써준다.
3. swap_table에 그 disk sector 추가
*/
	void *buffer;
	struct swap_entry *s;
  
	/* Allocate buffer. */
	buffer = malloc (DISK_SECTOR_SIZE);
	if (buffer == NULL)
	PANIC ("couldn't allocate buffer");
	memset (buffer, 0, DISK_SECTOR_SIZE);
	memcpy (buffer, paddr, PGSIZE);

	if(sector < disk_size(disk)){
		/* Write size to sector 1. */
		s = malloc (sizeof *s);
		s->disksector = sector;
		s->vaddr = vaddr;
		s->pid = pid;
		hash_insert(&swap_table, &s->hash_elem);
		disk_write (disk, sector++, buffer);
	}
	else{
		// disk capacity 만큼 한번 다 돈 경우
		s = swap_lookup_vaddr(NULL, (int)NULL);
		if (s == NULL){
			ASSERT(0);
			sys_exit(-1);
		}
		disk_write (disk, s->disksector, buffer);
	}
}



void
free_swap_slot_process(tid_t pid)
{
	struct hash_iterator i;

	hash_first (&i, &swap_table);
	while (hash_next (&i))
	{
		struct swap_entry *s = hash_entry (hash_cur (&i), struct swap_entry, hash_elem);
		if (s->pid == pid){
			s->vaddr = NULL;
			s->pid = (int)NULL;
		}
	}
}






/* swap_table을 돌면서 vaddr을 기준으로 검색 */
struct swap_entry *
swap_lookup_vaddr(const void *vaddr, tid_t pid)
{
	struct hash_iterator i;

	hash_first (&i, &swap_table);
	while (hash_next (&i))
	{
		struct swap_entry *s;
		s = hash_entry (hash_cur (&i), struct swap_entry, hash_elem);
		if (s->vaddr == vaddr && s->pid == pid){
			return s;
		}
	}
	return NULL;
}



















/* 
1. "Picking an unused swap slot" 
for evicting a page from its frame to the swap partition

void
pick_unused_slot()
{
	get_swap_entry(NULL);
}

2. "Freeing a swap slot" 
when its page is read back into frame / page가 swapped 된 process가 terminate

void
free_swap_slot(uint32_t vaddr)
{
	struct swap_entry * = get_swap_entry(vaddr);
	//free swap_entry
}
*/


/* Returns the swap_entry containing the given virtual address, 
	or a null pointer if no such frame exists 
struct swap_entry *
get_swap_entry(uint32_t vaddr, tid_t pid)
{
	struct hash_iterator i;

	//pid도 함께 비교해 줘야함
	hash_first (&i, frame_table);
	while (hash_next (&i))
	{
		struct frame *f = hash_entry (hash_cur (&i), struct frame, elem);
		if (f->vaddr == vaddr){
			return f;
		}
	}
	return NULL;

}
*/


