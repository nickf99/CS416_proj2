// File:	mypthread.c

// List all group member's name:Michael Nguyen, Nicholas Farinella
// username of iLab:mnn45
// iLab Server:atlas.cs.rutgers.edu

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
mypthread_t tidCount = 0; //keeps track of the number of threads and their IDs
#define StackSize 16384
#define ready 0
#define yield 1 
#define EXIT 2
#define join 3
#define waitMutex 4

#define QUANTUM 20000 //20 milliseconds
struct itimerval timer; 
long int sec = 0;
long int usec = 0;

struct sigaction sigToScheduler;

tcb *currentRunning = NULL;

//ready Queue
tcb *head = NULL; //points to the node containing the thread that is currently running

//mutex list
mypthread_mutex_t *mutexHead = NULL;
int mutexID = 0;

int OGThreadCreated = 0;
int doNotInterrupt = 0;

ucontext_t cleanup;

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
       // create Thread Control Block
       // create and initialize the context of this thread
       // allocate space of stack for this thread to run
       // after everything is all set, push this thread int
       // YOUR CODE HERE

	
	if (OGThreadCreated == 0){ //initialize the cleanup context. this will run when a thread ends
		memset(&cleanup, 0, sizeof(cleanup));
		getcontext(&cleanup);

		cleanup.uc_stack.ss_sp = malloc(StackSize);
		cleanup.uc_stack.ss_size = StackSize;
		cleanup.uc_link = NULL;
		makecontext(&cleanup, (void *)&exiting, 0);
	}

	doNotInterrupt = 1; // makes it so the timer doesn't mess up the creation
	//////INITIALIZES THE NEW TCB
	tcb *newTCB = (tcb *)malloc(sizeof(tcb));
	newTCB->TID = tidCount;	//initialize the thread id
	tidCount++;	//corrects the number of threads
	
	newTCB->status = ready;	//initialize the status

	// create the context to add to tcb
	newTCB->context = (ucontext_t *)malloc(sizeof(ucontext_t));
	getcontext(newTCB->context);
	newTCB->context->uc_stack.ss_sp = malloc(StackSize);
	newTCB->context->uc_stack.ss_size = StackSize;
	newTCB->context->uc_stack.ss_flags = 0;
	newTCB->context->uc_link = &cleanup;	
	makecontext(newTCB->context, (void *)function, 1, arg);
	//Above is to put the context in the newTCB

	newTCB->priority = 0; //priority is put in newTCB
	newTCB->joinValue = NULL;	//joinValue is added into newTCB
	newTCB->retValue = NULL;	//retValue is added into newTCB

	newTCB->elapsed = 0; //no quantums have been elapsed for this thread yet
	newTCB->next = NULL; //initialize next to NULL and add it into to the list in the next section
	

	addToQueue(newTCB);	//ADDS THE THREAD INTO QUEUE
	//////INITIALIZES THE TCB

	doNotInterrupt = 0; //creation of the new thread is complete

	//////CREATING THE ORIGINAL PROCESS THREAD
	if (OGThreadCreated == 0){	
		//if the original process has not been created yet
		//then create the thread and add it to the queue

		tcb *OGThread = (tcb *)malloc(sizeof(tcb));
		OGThread->TID = tidCount; //thread id is 1
		tidCount++; //there are 2 threads now
		OGThread->status = ready;


		//create the context for OGThread
		OGThread->context = (ucontext_t *)malloc(sizeof(ucontext_t));
		getcontext(OGThread->context);
		OGThread->context->uc_link = &cleanup;	//**********subject to change
		//create the context for OGThread


		//fill in the rest of the tcb
		OGThread->priority = 0;
		OGThread->joinValue = NULL;
		OGThread->retValue = NULL;
		OGThread->elapsed = 0;
		OGThread->next = NULL;
		//fill in the rest of the tcb

		currentRunning = OGThread; //The OGThread is currently running 
		OGThreadCreated = 1;

		/////////INITIALIZE THE SIGNAL HANDLER
		memset(&sigToScheduler, 0, sizeof(sigToScheduler));
		sigToScheduler.sa_handler = schedule;
		sigaction(SIGALRM, &sigToScheduler, NULL);

		
		//start the timer 
		timer.it_value.tv_sec = 0;
  		timer.it_value.tv_usec = QUANTUM;  
		timer.it_interval = timer.it_value;
		setitimer(ITIMER_REAL, &timer, NULL);
		
	}	
	//////CREATING THE ORIGINAL PROCESS THREAD


	

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// switch from thread context to scheduler context

	// YOUR CODE HERE
	if (OGThreadCreated == 0){ //since there is only one thread, there is none to yield to
		return -1; 
	}
	
	doNotInterrupt = 1;

	currentRunning->status = yield;

	doNotInterrupt = 0;
	raise(SIGALRM);


	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	
	// YOUR CODE HERE
	if (OGThreadCreated == 0){ //if OGThread has not been created yet then just exit 
		exit(0);
	}
	
	currentRunning->joinValue = value_ptr;
	setcontext(&cleanup);

};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) { //cannot join a thread that already has a thread waiting on it

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	if (OGThreadCreated == 0){ //if the OGThread is the only thread
		return -1;
	}

	if (thread == currentRunning->TID){ //if the thread calling join is joining itself
		return -1;
	}

	doNotInterrupt = 1;

	tcb *waitingOn = NULL; //this is the thread that is indicated by the thread argument
	tcb *ptr = head; //to traverse the ready queue
	tcb *joinPtr = head; //to traverse the joinQueue

	while (ptr != NULL){ //traverse all threads in ready queue
		joinPtr = ptr;
		while(joinPtr != NULL){	//start from the readyQueue node and then traverse all joinQueue nodes
			if (joinPtr->TID == thread){ //leave the inner loop if the TID's match
				break;
			}

			joinPtr = joinPtr->joinQueue;
		}
		if (joinPtr != NULL){ //if joinPtr is not NULL then it is on the desired thread
			waitingOn = joinPtr;
			break;
		}

		ptr = ptr->next;
	}// *******may have to implement the waitMutex list here as well 

	//for all mutexes in the mutexList 
	if (waitingOn == NULL){
		mypthread_mutex_t *mutexPtr = mutexHead;
		while(mutexPtr != NULL){
			ptr = mutexPtr->mutexQueue;
			while(ptr != NULL){
				if (ptr->TID == thread){
					break;
				}

				ptr = ptr->joinQueue;
			}
			if (ptr != NULL){
				waitingOn = ptr;
				break;
			}

			mutexPtr = mutexPtr->mutexList;
		}
	}

	

	if (waitingOn == NULL){ //this means that thread was not found 
		doNotInterrupt = 0;
		return -1;
	}

	if (waitingOn->joinQueue != NULL){ //this means that the thread has another thread already waiting on it
		doNotInterrupt = 0;
		return -1;
	}

	waitingOn->joinQueue = currentRunning; //current Running is in waitingOn's joinQueue
	currentRunning->status = join;
	doNotInterrupt = 0;

	raise(SIGALRM); //start running other threads, and will come back when the waitingOn thread has exited

	if (value_ptr == NULL){
		return 0;
	}
	else {
		*value_ptr = currentRunning->retValue;
	}


	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE

	if (mutex != NULL){ //if the mutex has already been initialized
		return -1;
	}

	doNotInterrupt = 1;
	mutex = (mypthread_mutex_t *)malloc(sizeof(mypthread_mutex_t)); //allocate memory
	mutex->initialized = 1; //the mutex is available
	mutex->locked = 0;	  //the mutex is not locked
	mutex->holder = -1;	  //nothing is holding the mutex right now
	mutex->ID = mutexID;
	mutexID++;

	//make a list of mutex
	mypthread_mutex_t *ptr = mutexHead;
	if (mutexHead == NULL){
		mutexHead = mutex;
	}
	else {
		while (ptr->mutexList != NULL){
			ptr = ptr->mutexList;
		}
		ptr->mutexList = mutex;
	}



	doNotInterrupt = 0;
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
	
	if (OGThreadCreated == 0){
		//initialize cleanup context
		memset(&cleanup, 0, sizeof(cleanup));
		getcontext(&cleanup);

		cleanup.uc_stack.ss_sp = malloc(StackSize);
		cleanup.uc_stack.ss_size = StackSize;
		cleanup.uc_link = NULL;
		makecontext(&cleanup, (void *)&exiting, 0);
		//initialize cleanup context


		/////////create the OGThread
		tcb *OGThread = (tcb *)malloc(sizeof(tcb));
		OGThread->TID = tidCount; //thread id is 0
		tidCount++; //there are 2 threads now
		OGThread->status = ready;


		//create the context for OGThread
		OGThread->context = (ucontext_t *)malloc(sizeof(ucontext_t));
		getcontext(OGThread->context);
		OGThread->context->uc_link = &cleanup;	//**********subject to change
		//create the context for OGThread


		//fill in the rest of the tcb
		OGThread->priority = 0;
		OGThread->joinValue = NULL;
		OGThread->retValue = NULL;
		OGThread->elapsed = 0;
		OGThread->next = NULL;
		//fill in the rest of the tcb

		currentRunning = OGThread; //The OGThread is currently running 
		OGThreadCreated = 1;

		/////////INITIALIZE THE SIGNAL HANDLER
		memset(&sigToScheduler, 0, sizeof(sigToScheduler));
		sigToScheduler.sa_handler = schedule;
		sigaction(SIGALRM, &sigToScheduler, NULL);

		//start the timer 
		timer.it_value.tv_sec = 0;
  		timer.it_value.tv_usec = QUANTUM;  
		timer.it_interval = timer.it_value;
		setitimer(ITIMER_REAL, &timer, NULL);
		/////////create the OGThread
	}

	doNotInterrupt = 1;

	if (mutex == NULL || mutex->initialized == 0){ //mutex has not been initialized or it is being destroyed
		doNotInterrupt = 0;
		return -1;
	}

	tcb *ptr = NULL;

	while (__atomic_test_and_set((volatile void *)&mutex->locked,__ATOMIC_RELAXED)){ //if true then mutex is locked
		
		doNotInterrupt = 1;

		//add current running to the mutex queue
		if (mutex->mutexQueue == NULL){
			mutex->mutexQueue = currentRunning;
		}
		else {
			/*
			ptr = mutex->mutexQueue;
			if (ptr->TID != currentRunning->TID){
				while (ptr->mutexQueue != NULL){
					if (ptr->mutexQueue->TID == currentRunning->TID){ //if the currentRunning is already in the mutexQueue
						break;
					}

					ptr = ptr->mutexQueue;
				}
				if (ptr->TID != currentRunning->TID && ptr->mutexQueue == NULL){ //insert currentRunning to the end of the mutexQueue if it is not already in the queue
					ptr->mutexQueue = currentRunning;
				}
			}
			*/
			ptr = mutex->mutexQueue;
			mutex->mutexQueue = currentRunning;
			currentRunning->mutexQueue = ptr;

		}
		//currentRunning is now in the mutexQueue
		currentRunning->status = waitMutex;

		doNotInterrupt = 0;
		raise(SIGALRM);
	}


	if(mutex == NULL || mutex->initialized == 0){ 
		doNotInterrupt = 0;
    	return -1;
  	}

	mutex->holder = currentRunning->TID;


	doNotInterrupt = 0;


    return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	if (OGThreadCreated == 0){ //lock initializes OGThread so that means the mutex is already unlocked 
		return -1;
	}


	doNotInterrupt = 1;

	if(mutex == NULL || mutex->initialized == 0 || mutex->locked == 0 || mutex->holder != currentRunning->TID){ //if the mutex is not initialized, lock is unlocked, or the calling thread is not the holder
		doNotInterrupt = 0;
		return -1;
	}

	mutex->holder = -1;
	mutex->locked = 0;


	//unqueue all of the threads waiting on the mutex and add them to the runQueue
	tcb *ptr = mutex->mutexQueue;
	tcb *temp = NULL;
	tcb *readyPtr = NULL;
	while (ptr != NULL){
		
		
		if (head == NULL || ptr->elapsed <= head->elapsed){ //if the thread should be placed at the head
			ptr->next = head;
			head = ptr;
		}
		else { //if the thread should be placed in the middle or the end 
			readyPtr = head;
			while (readyPtr->next != NULL){ //traverse the list for where ptr should be 
				if (ptr->elapsed <= readyPtr->next->elapsed){
					break;
				}
				readyPtr = readyPtr->next;
			}
			
			if (readyPtr->next = NULL){ //if readyPtr is at the end
				readyPtr->next = ptr;
			}
			else {	//if readyPtr is somewhere in the middle
				temp = readyPtr->next;
				readyPtr->next = ptr;
				ptr->next = temp;
			}
		}

		temp = ptr;
		ptr = ptr->mutexQueue;
		temp->mutexQueue = NULL;
		temp->status = ready;
	}
	mutex->mutexQueue = NULL;

	doNotInterrupt = 0;

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	if (mutex == NULL || mutex->initialized == 0){
		return -1;
	}

	doNotInterrupt = 1;

	mutex->initialized = 0;

	while (mutex->locked == 1){ //if it is locked then wait for it to unlock
		doNotInterrupt = 0;
		raise(SIGALRM);
	}
	 
	doNotInterrupt = 1;
	//put all of mutexQueue back into the readyQueue
	//unqueue all of the threads waiting on the mutex and add them to the runQueue
	tcb *ptr = mutex->mutexQueue;
	tcb *temp = NULL;
	tcb *readyPtr = NULL;
	while (ptr != NULL){
		
		if (head == NULL || ptr->elapsed <= head->elapsed){ //if the thread should be placed at the head
			ptr->next = head;
			head = ptr;
		}
		else { //if the thread should be placed in the middle or the end 
			readyPtr = head;
			while (readyPtr->next != NULL){ //traverse the list for where ptr should be 
				if (ptr->elapsed <= readyPtr->next->elapsed){
					break;
				}
				readyPtr = readyPtr->next;
			}
			
			if (readyPtr->next = NULL){ //if readyPtr is at the end
				readyPtr->next = ptr;
			}
			else {	//if readyPtr is somewhere in the middle
				temp = readyPtr->next;
				readyPtr->next = ptr;
				ptr->next = temp;
			}
		}

		temp = ptr;
		ptr = ptr->mutexQueue;
		temp->mutexQueue = NULL;
		temp->status = ready;
	}
	mutex->mutexQueue = NULL;

	mypthread_mutex_t *mptr = mutexHead;
	mypthread_mutex_t *mtemp = NULL;

	if (mptr->ID == mutex->ID){
		mutexHead = mutexHead->mutexList;
	}
	else {
		while (mptr->mutexList->ID != mutex->ID){
			mptr = mptr->mutexList;
		}
		mtemp = mptr->mutexList->mutexList;
		mptr->mutexList = mtemp;
	}
	

	//free mutex so it can be reinitialized	
	mypthread_mutex_t *tempMutex = mutex;
	free(tempMutex); 
	
	doNotInterrupt = 0;

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	//if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

	if (doNotInterrupt == 1){ //if the current running code does not want to be interrupted then return to it
		currentRunning->elapsed++;
		return;
	}
	
	//Stops the timer
	timer.it_value.tv_sec = 0;
  	timer.it_value.tv_usec = 0;  
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_REAL, &timer, NULL);
	//Stops the timer


	//send to the scheduling policy
	sched_stcf();

	return;
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
	
	currentRunning->elapsed++; //increment the elapsed time of the currently running thread
	//for all status cases
	if(currentRunning->status == ready){ // current running thread is ready
		if (head == NULL || currentRunning->elapsed <= head->elapsed){	//current running thread has run for less than or equal to the head of ready queue

			//start timer and go back to the current running thread
			timer.it_value.tv_sec = 0;
  			timer.it_value.tv_usec = QUANTUM;  
			timer.it_interval = timer.it_value;
			setitimer(ITIMER_REAL, &timer, NULL);
			return;
		}
		else {	//the head has run for less time than the current running thread

			tcb * oldCurrentRunning = currentRunning; //keeps track of the old current running 

			sortCRintoList(); //ready queue and current running are updated here

			//start timer and swap contexts
			timer.it_value.tv_sec = 0;
  			timer.it_value.tv_usec = QUANTUM;  
			timer.it_interval = timer.it_value;
			setitimer(ITIMER_REAL, &timer, NULL);

			//oldCurrentRunning context is saved into the tcb and the new thread will start running 
			swapcontext(oldCurrentRunning->context, currentRunning->context); 
			return;

		}
	}
	else if (currentRunning->status == yield){ //current running thread is yielding
		
		currentRunning->status = ready; //set current running back to ready

		if (head == NULL || currentRunning->elapsed < head->elapsed){ //currentRunning has the lowest elapsed in the whole queue

			//start timer and go back to the current running thread
			timer.it_value.tv_sec = 0;
  			timer.it_value.tv_usec = QUANTUM;  
			timer.it_interval = timer.it_value;
			setitimer(ITIMER_REAL, &timer, NULL);
			return;
		}
		else {	//there is atleast one thread that has the same or higher priority (currentRunning->elapsed >= head-elapsed)

			//put currentRunning in the queue after the last thread with the same elapsed
			tcb *oldCurrentRunning = currentRunning;

			yieldInsert();

			//start timer and swap contexts
			timer.it_value.tv_sec = 0;
  			timer.it_value.tv_usec = QUANTUM;  
			timer.it_interval = timer.it_value;
			setitimer(ITIMER_REAL, &timer, NULL);

			//oldCurrentRunning context is saved into the tcb and the new thread will start running 
			swapcontext(oldCurrentRunning->context, currentRunning->context); 
			return;
		}

	}
	else if (currentRunning->status == EXIT){ //current running thread is exiting

		if (currentRunning->joinQueue == NULL){ //if there is NOT a thread waiting for currentRunning to exit
			
			//free the node at currentRunning
			tcb *freeNode = currentRunning;
			free(freeNode->context->uc_stack.ss_sp);
			free(freeNode->context);
			free(freeNode);

			if (head == NULL){ //if head is NULL then the readyQueue is empty
				exit(0);
				//return;
			}
			//set up the next thread to run 
			currentRunning = head; 
			head = head->next;
			currentRunning->next = NULL;

			//start timer and start the next thread
			timer.it_value.tv_sec = 0;
  			timer.it_value.tv_usec = QUANTUM;  
			timer.it_interval = timer.it_value;
			setitimer(ITIMER_REAL, &timer, NULL);

			setcontext(currentRunning->context);

			return;
		}
		else { //if there is a thread waiting for currentRunning to exit

			tcb *freeNode = currentRunning;
			currentRunning = currentRunning->joinQueue; //currentRunning is now the thread that was waiting on the old currentRunning to exit

			//free old current running
			free(freeNode->context->uc_stack.ss_sp);
			free(freeNode->context);
			free(freeNode);

			currentRunning->status = ready;
			//the new currentRunning will now be sorted in the list and the new shortest elapsed thread will run
			if (head == NULL || currentRunning->elapsed <= head->elapsed){ //if readyQueue is empty or currentRunning has run for the shortest

				//start the timer and set context 
				timer.it_value.tv_sec = 0;
  				timer.it_value.tv_usec = QUANTUM;  
				timer.it_interval = timer.it_value;
				setitimer(ITIMER_REAL, &timer, NULL);

				setcontext(currentRunning->context);
				return;
			}
			else{ //currentRunning is not the shortest in the list
				sortCRintoList();
				
				//start the timer and set context 
				timer.it_value.tv_sec = 0;
  				timer.it_value.tv_usec = QUANTUM;  
				timer.it_interval = timer.it_value;
				setitimer(ITIMER_REAL, &timer, NULL);

				setcontext(currentRunning->context);
				return;
			}

		}



	}
	else if (currentRunning->status == join){ //current running thread is joining
		/*

		list may look like this:
		1--> 2--> 3-->
		v    v    v
		4    CR   

		*/

		//get the next thread to run
		tcb *oldCurrentRunning = currentRunning;
		currentRunning = head; 
		head = head->next;
		currentRunning->next = NULL;

		//start timer
		timer.it_value.tv_sec = 0;
  		timer.it_value.tv_usec = QUANTUM;  
		timer.it_interval = timer.it_value;
		setitimer(ITIMER_REAL, &timer, NULL);

		swapcontext(oldCurrentRunning->context, currentRunning->context);

		return;
	}
	else if (currentRunning->status == waitMutex){ //current running threads is waiting on a mutex lock
		
		//run the next thread
		tcb *oldCurrentRunning = currentRunning;
		currentRunning = head;
		
		

		if (currentRunning != NULL){ 
			head = head->next;
			currentRunning->next = NULL;

			timer.it_value.tv_sec = 0;
  			timer.it_value.tv_usec = QUANTUM;  
			timer.it_interval = timer.it_value;
			setitimer(ITIMER_REAL, &timer, NULL);

			swapcontext(oldCurrentRunning->context, currentRunning->context);
			return;
			
		}
		else{ //currentRunning is NULL so there is a deadlock
			exit(EXIT_FAILURE);
		}
		
		
	}

}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

void addToQueue(tcb *newTCB){	//adds a new TCB to the READY Queue

	if (head == NULL){	//if the list is empty then add it to the list and return
		head = newTCB; 
		return; //list will only consist of head
	}
	
	//if head is not null then newTCB is the new head
	newTCB->next = head; 
	head = newTCB;
	
	
	return;
}

void sortCRintoList(){ //puts a tcb into the list based off of elapsed time
	

	tcb *ptr = head; //to traverse the ready queue
	
	while (ptr->next != NULL){ //don't have to check head again 
		if (currentRunning->elapsed <= ptr->next->elapsed){ //if the ptr->next has ran for longer than current running then leave the loop
			break;
		}
		ptr = ptr->next;
	}
	//this will result in the ptr being somewhere in the middle or at the end of the list


	//current running should be inserted into ptr->next
	if (ptr->next == NULL){ //if ptr is at the end of the list 

		ptr->next = currentRunning; //add current running to the end

		currentRunning = head; //the new thread to run is the head
		head = head->next;	//the new head is the next node in the queue
		currentRunning->next = NULL;	//detatch the old head from the list

	}
	else { //if ptr is somewhere in the middle of the list

		tcb *greater = ptr->next;	//place holder for ptr->next
		ptr->next = currentRunning; //new ptr->next is the thread that just ran
		currentRunning->next = greater; //stitch up the list

		currentRunning = head; //the new thread to run is the head
		head = head->next;	//the new head is the next node in the queue
		currentRunning->next = NULL; //detatch the old head from the list
	}

	return;
}

void yieldInsert(){ //inserts current running into ready queue 

	//if currentRunning->elapsed is 3:
	//3->3->3->4->5
	//      ^this would be currentRunning

	tcb *ptr = head;

	while (ptr->next != NULL){ 
		if(currentRunning->elapsed < ptr->next->elapsed){ //if ptr is on the node right before the node that has a greater elapsed
			break;
		}

		ptr = ptr->next;
	}
	//ptr will not be at head because head has already been deemed to have less than or equal elapsed time than currentRunning

	//if ptr is at the end of the list
	if (ptr->next == NULL){
		ptr->next = currentRunning;
		
		currentRunning = head;
		head = head->next;
		currentRunning->next = NULL;
	}
	else { //if ptr is somewhere in the middle
		tcb *greater = ptr->next;
		ptr->next = currentRunning;
		currentRunning->next = greater;

		currentRunning = head;
		head = head->next;
		currentRunning->next = NULL;
	}


	return;
}

void exiting(){ //used to end exiting threads
	doNotInterrupt = 1;

	currentRunning->status = EXIT;

	if (currentRunning->joinQueue != NULL){ //if there is a thread that is waiting on currentRunning to end
		currentRunning->joinQueue->retValue = currentRunning->joinValue; //store value ptr here 
	}
	doNotInterrupt = 0;

	raise(SIGALRM);
}
