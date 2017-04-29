#include "vm/s-pagetable.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <list.h>
#include "vm/swap.h"
#include "vm/frame.h"

struct list s_page_table;

/*
  1. page fault가 난 page를 supplemental page table에 위치
  memory reference가 valid 이면 -> supplemental page table entry 를 이용하여 
  "file system" 혹은 "swap slot" 에 있을, 혹은 그냥 "all-zero page"를 locate
  (entry의 paddr == NULL 이면 
  메모리에 남은 자리가 있는지 보고 uint32_t free_frame = get_free_frame() 
  ( 이 함수 안에서 free frame없으면 알아서 swap out까지 해서 빈 프레임 넘겨줌 )
  위 함수 통과하면 free_frame은 빈 주소
	
  swap in(free_frame, va, pid)해야해 -> swaptable이 va, pid가지고 해당 disksector찾아)
  (if you implement sharing, page's data 는 이미 page frame에 있으나 page table에 없을 수 있다)

  2. page를 저장할 frame을 가져온다. (4.1.5 Managing Frame Table)
  (if you implement sharing, 우리가 필요한 data는 이미 frame에 있을 수 있다 -> you must be able to locate that frame)

  3. Fetch data into frame <- file system에서 가져오거나, swap하거나, zeroing it ...
  (if you implement sharing, page you need는 이미 frame에 있을 수 있다 -> no action is necessary)

  4. fault가 발생한 page table entry의 fulting virtul address-> physical page 이도록 만들어라 (userprog/pagedir.c) 
*/

void 
init_s_page_table(void)
{
	list_init(&s_page_table);
}




/* 
1. page fault가 발생했을 시 -> kernel이 s_pt에서 fault가 일어난 virtual page를 찾아,
what data should be there을 검색

2 process가 terminate 할 때 -> kernel이 s_pt에서 ,
what resources to free를 찾음 

*/

/*
	void s_pte_insert(void *va, void *pa, tid_t pid)
	from pagedir_set_page()
	
	1. when need add mapping, add mapping to supplemental page table also.
	2. latest one is always on the front, oldest one is always on the back.
	3. (va, pid) combination is the key.
	4. always add mapping to frame table too. (두 테이블을 맞게 유지해주기위해)

*/
void
s_pte_insert(void *va, void *pa, tid_t pid)
{
	//table entry에 va, pa, pid를 넣어서 table에 추가해준다.
	struct s_pte *pte;
	pte = malloc(sizeof *pte);
	pte->vaddr = va;
	pte->paddr = pa;
	pte->pid = pid;
	list_push_front(&s_page_table, &pte->elem);
	add_frame_entry(pa, va, pid);
}

/*
	void s_pte_clear(void *va, tid_t pid)
	from pagedir_clear_page()
	
*/
void
s_pte_clear(void *va, tid_t pid)
{
	//table entry 중에서 va, pid를 가진 애를 delete 함
	struct s_pte *entry = find_entry(va, pid);
	ASSERT (entry == NULL);
	entry->paddr = NULL;
	free_frame_entry(va, pid);		//frame table에서 해당 frame에 대한 mapping제거

}
/* 
	
	from page_fault()
	
	1. find the page entry that is not in physical memory now
*/
void
find_s_pte(void *vaddr, tid_t pid)
{
	/*
	va, pid를 가진 s_pagetable entry를 찾아서
	(entry의 paddr == NULL 이면 
  메모리에 남은 자리가 있는지 보고 uint32_t free_frame = get_free_frame() 
  ( 이 함수 안에서 free frame없으면 알아서 swap out까지 해서 빈 프레임 넘겨줌 )
  위 함수 통과하면 free_frame은 빈 주소
	
  swap in(free_frame, va, pid)해야해 -> swaptable이 va, pid가지고 해당 disksector찾아)

	*/

	struct s_pte *entry = find_entry(vaddr, pid);
	ASSERT (entry != NULL);
	if (entry->paddr == NULL)		//swap out된 경우
	{
		void *free_frame = get_free_frame();				//if memory full -> swap out
		swap_in(free_frame, vaddr, pid);		
	
	}
	
	
}


/* find a victim frmae with FIFO algorithm */
void *
get_victim(void)
{
	struct s_pte *pte;
	uint32_t victim;
	pte = list_entry(list_back(&s_page_table), struct s_pte, elem);
	victim = (uint32_t) (pte -> paddr); 				//WARNING
	pte->paddr = NULL;
	return (void *) victim;
}


struct s_pte *
find_entry(void *vaddr, tid_t pid)
{
	struct list_elem *e;
  	struct s_pte* pte;
  	for(e = list_begin(&s_page_table); e!= list_end(&s_page_table); e = list_next(e)){
	    pte = list_entry(e, struct s_pte, elem);
	    if (pte->pid == pid && pte->vaddr == vaddr){
	    	return pte;
	    }
  	}
  	return NULL;		// no entry found

}

/*
	when process terminate -> free entries in s_pt too
*/
