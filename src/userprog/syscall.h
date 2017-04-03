#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_arguments(char **, char *, int );
void sys_halt (void);
void sys_exit (int);
int sys_exec(const char *);
int sys_write(int, const void *, unsigned);
#endif /* userprog/syscall.h */
