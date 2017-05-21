#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "vm/s-pagetable.h"
#include "vm/frame.h"
#include "threads/pte.h"




/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
//ASSERT(0);
  /*
  1. page fault가 난 page를 supplemental page table에 위치
  memory reference가 valid 이면 -> supplemental page table entry 를 이용하여 
  "file system" 혹은 "swap slot" 에 있을, 혹은 그냥 "all-zero page"를 locate
  (if you implement sharing, page's data 는 이미 page frame에 있으나 page table에 없을 수 있다)

  2. page를 저장할 frame을 가져온다. (4.1.5 Managing Frame Table)
  (if you implement sharing, 우리가 필요한 data는 이미 frame에 있을 수 있다 -> you must be able to locate that frame)

  3. Fetch data into frame <- file system에서 가져오거나, swap하거나, zeroing it ...
  (if you implement sharing, page you need는 이미 frame에 있을 수 있다 -> no action is necessary)

  4. fault가 발생한 page table entry의 fulting virtul address-> physical page 이도록 만들어라 (userprog/pagedir.c) 
  */

  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */

  struct thread *curr = thread_current();
  //ASSERT(f->esp !=NULL);
  //printf("\n\n page fault \n\n");
  //printf("page fault addr %p, page addr %p, esp %p, pid %d\n", fault_addr, pg_round_down (fault_addr), f->esp, thread_current()->tid);

  if(fault_addr == NULL || is_kernel_vaddr(fault_addr))
  {
    //ASSERT(0);
    find_process(curr -> tid)->exit_status = -1;
    thread_exit();  
  }
  /* unmapped page */
  else if (pagedir_get_page(curr->pagedir, pg_round_down(fault_addr)) == NULL){
    /* ERROR */
    if(fault_addr == (f->esp)-PGSIZE)
    {
      //ASSERT(0);
      find_process(curr -> tid)->exit_status = -1;
      thread_exit();  
    }

    /* swap out & lazy loading */
    else if ( find_s_pte( pg_round_down (fault_addr), curr->tid )!= NULL) {
    }


    /* Stack Growth -  */
    else if (fault_addr >= f->esp - 32){
      //printf("PID : %d FAULT ADDRESS %p\n", curr ->tid, pg_round_down(fault_addr));
      find_process(curr -> tid) -> stack_end = pg_round_down(fault_addr);
      uint8_t *kpage;
      bool writable;
      kpage = palloc_get_page (PAL_USER | PAL_ZERO);
      //writable = pagedir_is_writable(curr->pagedir, pg_round_down(fault_addr));
      writable = true;
      if (kpage !=NULL)
      {
        /* aaaaaaa pt-grow-stk-sc */
        
        //printf("page fault addr %p, page addr %p, esp %p, esp page %p\n", fault_addr, pg_round_down (fault_addr), f->esp, pg_round_down(f->esp));
        lock_acquire(&evict_lock);
        pagedir_set_page (thread_current()->pagedir, pg_round_down(fault_addr), kpage, writable, -1);
        lock_release(&evict_lock);

      }
      else
      {
        lock_acquire(&evict_lock);
        kpage = ptov(get_free_frame());
        pagedir_set_page (thread_current()->pagedir, pg_round_down(fault_addr), kpage, writable, -1);
        lock_release(&evict_lock);
      }
      //ASSERT(0);
    }
    /* pt-bad-addr */
    else{
      //ASSERT(0);
      find_process(thread_current() -> tid)->exit_status = -1;
      thread_exit(); 
    }
      
  }
  

  /* pw-write-code2 */
  else{
 //ASSERT(0);
    find_process(thread_current() -> tid)->exit_status = -1;
    thread_exit(); 
    }
}

