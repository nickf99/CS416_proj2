// File:	mypthread.c

// List all group member's name:Michael Nguyen
// username of iLab:
// iLab Server:

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
mypthread_t tidCount = 0; //keeps track of the number of threads and their IDs
#define StackSize 16384
#define ready 0
#define yield 1 
#define exit 2
#define join 3
#define wait 4
#define waitMutex 5

#define QUANTUM 20000 //20 milliseconds
struct itimerval timer; 

struct sigaction sigToScheduler;

tcb *head = NULL; //points to the node containing the thread that is currently running

int OGThreadCreated = 0;
int doNotInterrupt = 0;

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
       // create Thread Control Block
       // create and initialize the context of this thread
       // allocate space of stack for this thread to run
       // after everything is all set, push this thread int
       // YOUR CODE HERE

	doNotInterrupt = 1; // makes it so the timer doesn't mess up the creation
	//////INITIALIZES THE NEW TCB
	tcb *newTCB = (tcb *)malloc(sizeof(tcb));
	newTCB->TID = tidCount;	//initialize the thread id
	tidCount++;	//corrects the number of threads
	
	newTCB->status = ready;	//initialize the status

	// create the context to add to tcb
	memset(&newTCB, 0, sizeof(newTCB));
	getcontext(&newTCB);
	newTCB->context.uc_stack.ss_sp = malloc(StackSize);
	newTCB->context.uc_stack.ss_size = StackSize;
	newTCB->context.uc_stack.ss_flags = 0;
	newTCB->context.uc_link = NULL;	
	makecontext(&newTCB, (void *)function, 1, arg);
	//Above is to put the context in the newTCB

	newTCB->priority = 0; //priority is put in newTCB
	newTCB->creatorThread = NULL;	//creatorThread is added into newTCB
	newTCB->createdThread = NULL;	//createdThread is added into newTCB

	newTCB->elapsed = 0; //no quantums have been elapsed for this thread yet
	newTCB->next = NULL; //initialize next to NULL and add it into to the list in the next section
	

	addToQueue(newTCB);	//ADDS THE THREAD INTO QUEUE
	//////INITIALIZES THE TCB


	//////CREATING THE ORIGINAL PROCESS THREAD
	if (OGThreadCreated == 0){	
		//if the original process has not been created yet
		//then create the thread and add it to the queue

		tcb *OGThread = (tcb *)malloc(sizeof(tcb));
		OGThread->TID = tidCount; //thread id 
		tidCount++; //there are 2 threads now
		OGThread->status = ready;


		//create the context for OGThread
		memset(&OGThread, 0, sizeof(OGThread));
		getcontext(&OGThread);
		OGThread->context.uc_link = NULL;	//**********subject to change
		//create the context for OGThread


		//fill in the rest of the tcb
		OGThread->priority = 0;
		OGThread->creatorThread = NULL;
		OGThread->createdThread = NULL;
		OGThread->elapsed = 0;
		OGThread->next = NULL;
		//fill in the rest of the tcb

		addToQueue(OGThread); //OGThread is now the head of the list 

		OGThreadCreated = 1;


		/////////INITIALIZE THE SIGNAL HANDLER
		sigToScheduler.sa_handler = schedule;
		sigaction(SIGALRM, &schedule, NULL);

		/////////START THE TIMER ***********maybe not here, might be in the scheduler to start the timer
		timer.it_value.tv_sec = 0; 
  		timer.it_value.tv_usec = QUANTUM; 
		timer.it_interval = timer.it_value;

		//raise(SIGALRM); //maybe just signal to go to scheduler after the threads are created 
	}	
	//////CREATING THE ORIGINAL PROCESS THREAD


	doNotInterrupt = 0; //makes it so the scheduler does not interrupt in these areas

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

	if (doNotInterrupt == 1){ //if the current running code does not want to be interrupted then return to it
		return;
	}



// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

void addToQueue(tcb *newTCB){	//adds a new TCB to the node after the one currently running

	if (head == NULL){	//if the list is empty then add it to the list and return
		head = newTCB; 
		return; //list will only consist of head
	}
	if (head->next == NULL){ //if the node after the one currently running is NULL then add it to the end of the list
		head->next = newTCB;
		return; //list will look like: head -----> newTCB
	}

	//make it so the list looks like: 
	//head -----> newTCB -----> temp -----> the rest of the list
	tcb *temp = head->next; 
	head->next = newTCB;
	newTCB->next = temp;
	
	return;
}
