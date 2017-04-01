#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}



/* System Calls (3.5.2 System Call Details)
1. Retrieve the system call number (3.1.5 Accessing User Memory)
  System call number is on the user's stack in the user's virtual address space
  1) (is_valid_usraddr()) -> if is invalid pointer : teminate user process

2. Synchronize system call (any number of user processes can make them at once)
  + treat file system code as a critical section (process_execute() also accesses files)

3. Call proper syscall function (convention for function return values is to place them in the EAX register intr_frame->eax)


+ Sole way a user program should be able to cause the OS to halt is by invoking the 'halt' system call
+ If a system call is passed an invalid argument, acceptable options include returning an error value (for those calls that return a value),
  returning an undefined value, or terminating the process
*/
static void
syscall_handler (struct intr_frame *f) 
{
  
  //for synchronization
  //intr_disable();			
  printf ("system call!\n");
  
  //1. Retrieve the system call number
  /*caller's stack pointer의 32-bit word에 system call number 존재
  next higher address 에 다음 argument 들이 있음
  */
/*
  if(!is_valid_usraddr(f->esp)){
  	//terminate user process;
  }

  char *sp;
  sp = (char *) malloc (sizeof(char *));
  memcpy(sp, f -> esp, sizeof(char *));
  hex_dump ((uintptr_t) (sp-100), (void **) (sp-100), 200, true);

  uint32_t syscall_number;

  syscall_number = *sp;
  char **arguments;
  arguments = (char **) malloc ( 4 * sizeof(char*));

  switch (syscall_number) {
  
  case SYS_HALT :
  	sys_halt();
  	break;

  case SYS_EXIT :
  	sys_exit(args[0]);
  break;

  case SYS_EXEC :                   
  	sys_exec(args[0]);
  	eax;
  break;

  case SYS_WAIT :
  	sys_wait(args[0]);
  	eax;
  	break;        

  case SYS_CREATE :                
  	sys_create(args[0], args[1]);
  	eax;
  	break;

  case SYS_REMOVE :                
  	sys_remove(args[0]);
  	eax;
  	break;

  case SYS_OPEN :               
  	sys_open(args[0]);
  	eax;
  	break;

  case SYS_FILESIZE :
  	sys_filesize(args[0]);;
  	eax;
  	break;

  case SYS_READ :          
  	sys_read(args[0], args[1], args[2]);
  	eax;
  	break;

  case SYS_WRITE : 
  	sys_write(args[0], args[1], args[2]);
  	eax;
  	break;

  case SYS_SEEK :
  	sys_seek(args[0], args[1]);  
  	eax;          
  	break;

  case SYS_TELL :     
  	sys_tell(args[0]);
  	eax;
  	break;

  case SYS_CLOSE :   
  	sys_close(args[0]);
  	break;
 }
 */
  thread_exit ();
}





