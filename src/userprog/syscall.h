#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdint.h>

void syscall_init (void);
void syscall_arguments(uint32_t **, uint32_t *, int);
void sys_halt (void);
void sys_exit (int);
int sys_exec(const char *);
int sys_write(int, const void *, unsigned);
#endif /* userprog/syscall.h */

