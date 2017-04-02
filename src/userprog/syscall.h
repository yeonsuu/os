#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_arguments(char **, char *, int );
void sys_exit (int);
#endif /* userprog/syscall.h */
