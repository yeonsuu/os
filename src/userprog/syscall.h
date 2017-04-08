#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdint.h>
#include <stdio.h>
#include "threads/thread.h"

struct lock filesys_lock;

void syscall_init (void);
void syscall_arguments(uint32_t **, uint32_t *, int);
void sys_halt (void);
void sys_exit (int);
int sys_exec(const char *);
int sys_write(int, const void *, unsigned);
bool sys_create(const char *, unsigned);
int sys_wait(tid_t);
int sys_open(const char *);
int sys_read(int, const void *, unsigned);
int sys_filesize(int);
void sys_seek(int, unsigned);
bool sys_remove(const char *);

unsigned sys_tell(int);
void sys_close(int);


#endif /* userprog/syscall.h */

