
#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
/*
struct process
{
	tid_t pid;
	struct semaphore sema_pexit;
	struct semaphore sema_pexec;
	int exit_code;		// if this process dead-> 
	struct list_elem elem;
}
*/
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void set_exitstatus(int);
int get_exitstatus(tid_t);
bool is_valid_usraddr (void *);
#endif /* userprog/process.h */
