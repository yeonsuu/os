#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "devices/input.h"


struct lock filesys_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
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
  
  /* 
  1. For SYNCHRONIZATION
  enum intr_level old_level;
  */

  uint32_t *sp = f->esp;
  uint32_t *argv[3];

  if(!is_valid_usraddr(sp)){
  	sys_exit(-1);
  }

  switch (*sp) {
  
  case SYS_HALT :
  	sys_halt();
  	break;

  case SYS_EXIT :
    syscall_arguments(argv, sp, 1);
  	sys_exit((int)*argv[0]);
  break;

  case SYS_EXEC :   
    syscall_arguments(argv, sp, 1);          
  	f -> eax = sys_exec((char *)*(uint32_t *)argv[0]);
  break;

  case SYS_WAIT :
    syscall_arguments(argv, sp, 1);
  	f -> eax = sys_wait((tid_t)*argv[0]);
  	break;        

  case SYS_CREATE :
    syscall_arguments(argv, sp, 2);               
  	f -> eax = sys_create((char *)*(uint32_t *)argv[0], (unsigned)*argv[1]);
  	break;

  case SYS_REMOVE :   
    syscall_arguments(argv, sp, 1);     
  	f -> eax = sys_remove((char *)*(uint32_t *)argv[0]);
  	break;

  case SYS_OPEN :  
    syscall_arguments(argv, sp, 1);
  	f->eax = sys_open((char *)*(uint32_t *)argv[0]);
  	break;

  case SYS_FILESIZE :
    syscall_arguments(argv, sp, 1);
  	f->eax = sys_filesize((int)*argv[0]);
  	break;

  case SYS_READ :   
    syscall_arguments(argv, sp, 3);
  	f->eax = (off_t) sys_read((int)*argv[0], (void *)*(uint32_t *)argv[1], (unsigned) * argv[2]);
  	break;

  case SYS_WRITE : 
    syscall_arguments(argv, sp, 3);
  	f->eax = sys_write((int)*argv[0], (void *)*(uint32_t *)argv[1], (unsigned) *argv[2]);
  	break;

  case SYS_SEEK :
    syscall_arguments(argv, sp, 2);
    sys_seek((int)*argv[0], (unsigned)*argv[1]);
  	break;

  case SYS_TELL :  
    syscall_arguments(argv, sp, 1);
  	f->eax = sys_tell((int)*argv[0]);
  	break;

  case SYS_CLOSE : 
    syscall_arguments(argv, sp, 1);
  	sys_close((int)*argv[0]);
  	break;
 }
 
}


void
syscall_arguments(uint32_t **argv, uint32_t *sp, int argc) {
  int i;
  for (i = 0; i < argc; i++)
  {
    sp++;
    if(!is_valid_usraddr((void *)sp))
      sys_exit(-1);  
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
//printf("!!!syscall : exit!!!\n");

	set_exitstatus(status);
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
  	  //printf("!!!syscall : exec!!!\n");
  if(!is_valid_usraddr((void *)cmd_line))
    return -1;
  else{
    int result = process_execute(cmd_line);
    return result;
  }


}
/* Waits for a child process pid and retrieves the child's exit status */

int
sys_wait(tid_t pid)
{
  //printf("!!!!!sys wait!!!!1\n");
  return process_wait(pid);
}

/*
Creates a new file called file, initially initial_size bytes in size
RETURN successful -> true / else -> false
ONLY CREATE. OPENING NEW FILE -> SYS_OPEN
*/

bool
sys_create(const char *file, unsigned initial_size)
{
  if(file == NULL)
    sys_exit(-1);
  if(!is_valid_usraddr((void *)file))
      sys_exit(-1);  
  return filesys_create(file, initial_size);
}
bool
sys_remove(const char *file)
{
  if(file == NULL)
    sys_exit(-1);
  if(!is_valid_usraddr((void *)file))
    sys_exit(-1);
  return filesys_remove(file);  
}
int
sys_open(const char *file)
{
  struct fd_file *fd_and_file;
  fd_and_file = malloc (sizeof *fd_and_file);
  int fd;
  struct file * f;
  struct process * p;
  p = find_process(thread_current()->tid);

  if(file == NULL)
    sys_exit(-1);
  if(!is_valid_usraddr((void *)file))
    sys_exit(-1);

  f = filesys_open (file);
  //printf("open file length %d\n", file_length(f));
  if(f == NULL){
    fd = -1;
  }
  else{
    fd = p->fd_cnt;
    p->fd_cnt++;

    fd_and_file->fd = fd;
    fd_and_file->file = f;

    
    list_push_back(&p->file_list, &fd_and_file->elem);
  }
  return fd;
}

int
sys_filesize(int fd)
{
  struct file *f = find_file(fd)->file;

  int length = file_length (f);

  return length;
}

int
sys_read(int fd, const void *buffer, unsigned size)
{

  struct file *f;
  if((void *)buffer == NULL){
    sys_exit(-1);
  }
  if(!is_valid_usraddr((void *)buffer) || !is_user_vaddr (buffer + size)){
    sys_exit(-1);
  }
  if (fd == 1){
    sys_exit(-1);
  }
  lock_acquire(&filesys_lock);
  int result;

  if(fd == 0){
    int i;
    for (i = 0; i !=(int)size; i++){
      *(uint8_t *)buffer = input_getc();
      buffer++;
    }
    result = size;
    lock_release(&filesys_lock);
  }

  else{
    if (find_file(fd) == NULL){
      lock_release(&filesys_lock);
      sys_exit(-1);
    }
    
    f = find_file(fd)->file;
    result = file_read(f, buffer, (off_t) size);
    lock_release(&filesys_lock);
  }
  return result;
}

/* return the number of bytes actually written*/
int
sys_write(int fd, const void *buffer, unsigned size)
{
  struct file *f;

  if(!is_valid_usraddr((void *)buffer) || !is_user_vaddr (buffer + size)){
    sys_exit(-1);
  }
  if (fd == 0){
    sys_exit(-1);
  }

  lock_acquire(&filesys_lock);
  int result;
  if(fd == 1){
    putbuf (buffer, size);
    result = size;
  }  
  else{
    if (find_file(fd) == NULL){
      lock_release(&filesys_lock);
      sys_exit(-1);
    }
    
    f = find_file(fd)->file;
    result = file_write(f, buffer, (off_t) size);
  }
  lock_release(&filesys_lock);

  return result;
}
void
sys_seek(int fd, unsigned position)
{
  struct file *f;
  if (fd == 0){
    ASSERT(0);
  }
  if(fd == 1){
    ASSERT(0);
  }  

  if (find_file(fd) == NULL){
    sys_exit(-1);
  }
  f = find_file(fd)->file;
    
  file_seek(f, position);
  return;
}

unsigned
sys_tell(int fd)
{
  struct file *f;
  if (fd == 0){
    ASSERT(0);
  }
  if(fd == 1){
    ASSERT(0);
  }  
  if (find_file(fd) == NULL){
    sys_exit(-1);
  }
  f = find_file(fd)->file;
    
  return file_tell(f);
}

void
sys_close(int fd)
{
  if (fd == 0)
    sys_exit(-1);
  if (fd == 1)
    sys_exit(-1);
  if (find_file(fd) == NULL){
    sys_exit(-1);
  }
  struct file *f = find_file(fd)->file;
  
  if (f == NULL)
    sys_exit(-1);

  else{
    file_close (f);
    list_remove(&find_file(fd)->elem);
  }
}

