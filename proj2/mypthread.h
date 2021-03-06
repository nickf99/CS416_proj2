// File:	mypthread_t.h

// List all group member's name: Michael Nguyen
// username of iLab:
// iLab Server:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

typedef uint mypthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	mypthread_t TID;

	//0 = ready state, 1 = yield state, 2 = exit state, 3 = join state, 4 = wait state, 5 is the waiting for mutex state
	int status;
	ucontext_t *context;	//contains the context for the thread

	int priority;
	void *joinValue;
	void *retValue;

	int elapsed; //elapsed quantums

	struct threadControlBlock *next; //ptr to next thread in the list
	struct threadControlBlock *joinQueue; //ptr to the first thread that calls join for this thread
	struct threadControlBlock *mutexQueue; //ptr to threads that are waiting on this thread to unlock a mutex
} tcb;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	/* add something here */
	int locked; 
	int holder;
	int initialized;
	int ID;
	struct mypthread_mutex_t *mutexList; //list of mutexes
	tcb *mutexQueue; //threads waiting on the mutex to unlock

	// YOUR CODE HERE
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE
typedef struct threadQueue {
	tcb *thread;
	struct threadQueue *next;

}threadList;


/* Function Declarations: */
void addToQueue(tcb *newTCB);
void sortCRintoList();
void yieldInsert();
void exiting();
static void sched_stcf();
static void schedule();

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
