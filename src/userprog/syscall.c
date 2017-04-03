#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/malloc.h"


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
  struct thread *curr = thread_current();
  //for synchronization
  
  //1. Retrieve the system call number
  /*caller's stack pointer의 32-bit word에 system call number 존재
  next higher address 에 다음 argument 들이 있음
  */

  /*if(!is_valid_usraddr(f->esp)){
    
  	thread_exit ();
  }*/
  char *sp;
  sp = (char *) malloc (sizeof(char *));
  //memcpy(sp, f->esp, sizeof(char *));
  sp = f->esp;
  
  //hex_dump ((uintptr_t) (f->esp -100), (void **) (sp-100), 200, true);

  uint32_t syscall_number;

  syscall_number = *sp;
  char **argv;
  argv = (char **) malloc ( 3 * sizeof(char*));

  switch (syscall_number) {
  
  case SYS_HALT :
  	sys_halt();
  	break;

  case SYS_EXIT :
    syscall_arguments(argv, sp, 1);
  	sys_exit((int)*argv[0]);
  break;

  case SYS_EXEC :   
    syscall_arguments(argv, sp, 1);          
  	f -> eax = sys_exec((char *)*argv[0]);
  break;

  case SYS_WAIT :
    syscall_arguments(argv, sp, 1);
  	//sys_wait(argv[0]);
  	break;        

  case SYS_CREATE :
    syscall_arguments(argv, sp, 2);               
  	//sys_create(argv[0], argv[1]);
  	break;

  case SYS_REMOVE :   
    syscall_arguments(argv, sp, 1);     
  	//sys_remove(argv[0]);
  	break;

  case SYS_OPEN :  
    syscall_arguments(argv, sp, 1);
  	//sys_open(argv[0]);
  	break;

  case SYS_FILESIZE :
    syscall_arguments(argv, sp, 1);
  	//sys_filesize(argv[0]);
  	break;

  case SYS_READ :   
    syscall_arguments(argv, sp, 3);
  	//sys_read(argv[0], argv[1], argv[2]);
  	break;

  case SYS_WRITE : 
    //printf("%d\n", *(sp+4));

    syscall_arguments(argv, sp, 3);

    //printf("1 : %d, 2: %p, 3: %d", *argv[0], *((int *)argv[1]), (unsigned)*argv[2]);
    //hex_dump ((uintptr_t) (sp -100), (void **) (sp-100), 200, true);

  	f->eax = sys_write((int)*argv[0], *((int *)argv[1]), (unsigned)*argv[2]);
  	break;

  case SYS_SEEK :
    syscall_arguments(argv, sp, 2);
  	//sys_seek(argv[0], argv[1]);  
  	break;

  case SYS_TELL :  
    syscall_arguments(argv, sp, 1);
  	//sys_tell(argv[0]);
  	break;

  case SYS_CLOSE : 
    syscall_arguments(argv, sp, 1);
  	//sys_close(argv[0]);
  	break;
 }
 
  //thread_exit ();
}


void
syscall_arguments(char **argv, char *sp, int argc) {
  int i;
  for (i = 0; i < argc; i++)
  {
    sp+=4;
    if(!is_valid_usraddr((void *)sp)){    //실제로 여기 들어가면 안됨
      sys_exit(-1);  
    }
    argv[i] = sp;
    
  }
}






/* Terminate Pintos */
void
sys_halt(void)
{
  power_off();
}

/* Terminate current user program. Return status to the kernel
If the process's parent waits for it, this is the status that will be returned
A status of 0 indicates success and nonzero values indicate errors */
void
sys_exit(int status)
{

  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}





/* Runs the executable whose name is given in cmd_line, and returns the new process's program id (pid)
   Return pid -1 if program cannot load or run 
   -> process_execute(cmd_line)

parent process cannot return from the exec until it knows whether the child process successfully loaded its excutable 
==> SYNCHRONIZATION 

INPUT : excecutable's name

OUTPUT : new process's program id (pid)
*/
int
sys_exec(const char *cmd_line)
{
  ASSERT(0);
   return process_execute(cmd_line);


}
/* Waits for a child process pid and retrieves the child's exit status */
/*
int
sys_wait(pid_t pid)
{

  eax;
}
*/
/*
void
sys_create(args[0], args[1])
{
  eax;
}

void
sys_remove(args[0])
{
  eax;
}

void
sys_open(args[0])
{
  eax;
}

void
sys_filesize(args[0])
{
  eax;
}

void
sys_read(args[0], args[1], args[2])
{
  eax;
}
*/
/* return the number of bytes actually written*/
int
sys_write(int fd, const void *buffer, unsigned size)
{
  int bytes = 0;
  if (fd == 1){
    putbuf (buffer, size);
    bytes = size;
  }
  return bytes;
}
/*
void
sys_seek(args[0], args[1])
{
  eax;
}

void
sys_tell(args[0])
{
  eax;
}

void
sys_close(args[0])
{

}
*/
