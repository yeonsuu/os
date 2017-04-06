
#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <list.h>
#include "threads/synch.h"

struct process
{
	tid_t pid;
	tid_t child_pid;
	tid_t parent_pid;
	struct semaphore sema_pwait;
	struct semaphore sema_pexec;
	int exit_status;		// if this process dead-> 
	struct list_elem elem;
	bool load_success;
	bool is_dead;
};

void process_init (void);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void set_exitstatus(int);
int get_exitstatus(tid_t);
struct list_elem * find_processelem(tid_t);
struct process * find_process(tid_t);
bool is_valid_usraddr (void *);
#endif /* userprog/process.h */
