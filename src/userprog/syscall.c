#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include <string.h>

#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "vm/frame.h"
#include "vm/s-pagetable.h"
#include "vm/swap.h"
#include "vm/file-table.h"
#include "vm/mmap-table.h"


static void syscall_handler (struct intr_frame *);
struct lock filesys_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
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

    case SYS_MMAP :
      syscall_arguments(argv, sp, 2);
      f->eax = sys_mmap((int)*argv[0], (void*)*argv[1]);
      break;

    case SYS_MUNMAP :
      syscall_arguments(argv, sp, 1);
      sys_munmap((int)*argv[0]);
      break;
  }
}


void
syscall_arguments(uint32_t **argv, uint32_t *sp, int argc) {
  int i;

  for (i = 0; i < argc; i++){
    sp++;
    if(!is_valid_usraddr((void *)sp))
      sys_exit(-1);  
    argv[i] = sp;
  }
}

void
sys_halt(void)
{
  power_off();
}

void
sys_exit(int status)
{
	set_exitstatus(status);
  thread_exit();
}

int
sys_exec(const char *cmd_line)
{
  if(!is_valid_usraddr((void *)cmd_line))
    sys_exit(-1);
  else
    return process_execute(cmd_line);
}

int
sys_wait(tid_t pid)
{
  return process_wait(pid);
}

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
  if(file == NULL)
    sys_exit(-1);
  if(!is_valid_usraddr((void *)file))
    sys_exit(-1);
    
  int fd;
  struct file * f;
  struct process * p;
  p = find_process(thread_current()->tid);
  lock_acquire(&filesys_lock);
  f = filesys_open (file);
lock_release(&filesys_lock);
  //ERROR: file is NULL
  if(f == NULL){
    fd = -1;
  }
  
  //ADD file&fd to current process's file_list
  else{
    fd = p->fd_cnt;
    p->fd_cnt++;

    struct fd_file *fd_and_file;
    fd_and_file = malloc (sizeof *fd_and_file); 

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
  return file_length (f);
}

int
sys_read(int fd, const void *buffer, unsigned size)
{
  struct file *f;
  int result;

  if((void *)buffer == NULL){
    sys_exit(-1);
  }
  //CASE 0: fd == 1 is write to command
  if (fd == 1){
    sys_exit(-1);
  }

  //Filesys synchronization
  lock_acquire(&filesys_lock);

  //CASE 1: READ from command
  if(fd == 0){
    int i;
    for (i = 0; i !=(int)size; i++){
      *(uint8_t *)buffer = input_getc();
      buffer++;
    }
    lock_release(&filesys_lock);
    return size;
  }
  //CASE 2: READ from file
  else{
    //ERROR: NO FILE!
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

int
sys_write(int fd, const void *buffer, unsigned size)
{
  struct file *f;

  //CASE 0: buffer's address is NOT available
  if(!is_valid_usraddr((void *)buffer) || !is_user_vaddr (buffer + size)){
    sys_exit(-1);
  }
  //CASE 0: fd == 0 is read to command
  if (fd == 0){
    sys_exit(-1);
  }

  lock_acquire(&filesys_lock);
  int result;

  //CASE 1: WRITE to command
  if(fd == 1){
    putbuf (buffer, size);
    result = size;
  }
  //CASE 2: WRITE to file
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
  //ERROR: NOT FILE(COMMAND)
  if (fd == 0)
    ASSERT(0);
  if(fd == 1)
    ASSERT(0);
  //ERROR: CANNOT find file
  if (find_file(fd) == NULL)
    sys_exit(-1);

  f = find_file(fd)->file;  
  file_seek(f, position);
  return;
}

unsigned
sys_tell(int fd)
{
  struct file *f;
  //ERROR: NOT FILE(COMMAND)
  if (fd == 0)
    ASSERT(0);
  if(fd == 1)
    ASSERT(0);
  //ERROR: CANNOT find file
  if (find_file(fd) == NULL)
    sys_exit(-1);
  f = find_file(fd)->file;

  return file_tell(f);
}

void
sys_close(int fd)
{
  //ERROR: NOT FILE(COMMAND)
  if (fd == 0)
    sys_exit(-1);
  if (fd == 1)
    sys_exit(-1);
  //ERROR: CANNOT find file
  if (find_file(fd) == NULL){
    sys_exit(-1);
  }
  struct file *f = find_file(fd)->file;
  
  //ERROR: FILE is NULL
  if (f == NULL){
    list_remove(&find_file(fd)->elem);
    free(find_file(fd));
    sys_exit(-1);
  }
  else{
    file_close (f);
    list_remove(&find_file(fd)->elem);
    free(find_file(fd));
  }
}


/*
Maps the file open as fd into the process's virtual address space
entire file 은 addr로 시작하는 연속적인 virtual pages에 map

mmap region에 있는 page들을 lazily load 해야한다.
그리고 mmaped file을 backing store for the mapping 으로 사용
(evicting a page mapped by mmap writes it backto the file it was mapped from)

file 길이가 PGSIZE의 배수가 아니면, 나머지는 0으로 채워서 읽어오고, 다시 작성할 때는 0은 쓰지 않는다.

*/
int 
sys_mmap(int fd, void *upage)
{
  struct file *file = find_file(fd)->file;
  int length = file_length(file);
  bool writable = true;

  //printf("mmap page %p\n", upage);
  if(length == 0){
    return -1;
  }

  if (upage != pg_round_down(upage)){
    return -1;
  }

  if(upage == NULL){
    return -1;
  }
  if(fd == 0 || fd == 1){
    return -1;
  }
  if (is_kernel_vaddr(upage + length)){
    return -1;
  }
  /* if range 가 existing set of mapped pages 라면 (executable도 포함)
  */
  struct process * p = find_process(thread_current()->tid);
  if (find_mapping_vaddr(&p->mapping_list, upage) != NULL){
    return -1;
  }
  if (!is_valid_mapping_load(&p->load_file_table, upage))
  {
    return -1;
  }

  if ((upage <= p->stack_start) && (upage >= p->stack_end))
  {
    return -1;
  }

  struct mapping *m;
  m = malloc(sizeof *m);
  m -> start = upage;
  m -> size = length;
  m -> file = file_reopen(file);
  m -> fd = fd;
  m -> id = list_size(&p->mapping_list) + 1;    // start from 1 for the first one
  //list에서 사라지는 경우 (munmap일 경우에)
/*
  if (list_size(&p->mapping_list) == 0)
    m-> id = 0;
  else
    fte = list_entry(list_rbegin(file_table), struct fte, elem);
  */

  list_init(&m->file_table);
  list_push_back(&p->mapping_list, &m->elem);
  int read_bytes = length;
  int ofs = 0;
  //printf("mmap : id %d\n", m->id);

  while (read_bytes > 0 ) 
    {
       
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;
      
      uint8_t *kpage = palloc_get_page (0);
      s_pte_insert(upage, NULL, thread_current()->tid, m->id);
      file_insert(&m->file_table, upage, ofs, page_read_bytes, writable);
       if (file_read (m->file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
                    ASSERT(0);

          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      palloc_free_page(kpage);
      //printf("file->pos : %p\n", m->file->pos);
      read_bytes -= page_read_bytes;
      upage += PGSIZE;
      ofs += page_read_bytes;


    }

    return m->id;

}


void 
sys_munmap(int mapping)
{
  
  struct process * p = find_process(thread_current()->tid);
  struct mapping * m = find_mapping_id(&p->mapping_list, (int)mapping);
  if(m == NULL){
    sys_exit(-1);
  }
  //printf("mummap : id %d\n", mapping);
  //lock_acquire(&filesys_lock);
  is_written(&m->file_table, m->fd);
  file_close(m->file);
  //lock_release(&filesys_lock);
  free_mapping(&m->file_table);
  //list_remove(&m->elem);
  //free(m);

}

