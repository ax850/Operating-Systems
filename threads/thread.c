#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
/* Define Queues */
typedef struct Queue {	/* Creating Doubly Linked List */
	
	struct node * head;
	struct node * tail;	
	int que_size;
	
}Queue;

struct node {
	struct node * next;		// Points to next node (Thread)
	struct thread *thread_pointer;	// Points to the Thread
}node;

typedef enum thread_status{
	threadRun,
	threadExit,
	threadAwake,
	threadReady,
	threadSleep 		// Lab3 requires Sleep Status
	
}thread_status;


/* This is the thread control block */
typedef struct thread {
	/* ... Fill this in ... */
	int thread_id;
	ucontext_t* mycontext;
	thread_status mystatus;
	void * stack_p;
}thread;

/* Queue's must be defined as global */
Queue readyQue;
Queue exitQue;


/* Functions For Queue */
void add_to_queue(Queue *Que, thread *thread_);
void remove_from_queue (Queue *Que, int n);
int get_running_thread_id(Queue *Que);
thread * get_running_thread(Queue *Que);
void move_up(Queue* Que, int pos);

void thread_destroy();

int id_exists(Queue *Que, int count);
void thread_stub(void (*thread_main)(void *), void *arg);

thread kernal;
ucontext_t kernal_context;
struct node node1;

void
thread_init(void)
{
	/* your optional code here */
	/* Initialize Ready and Exit Queue's */
	
	exitQue.head = NULL;
	exitQue.tail = NULL;
	exitQue.que_size = 0;
	
	
	
	kernal.thread_id = 0;
	kernal.mycontext = &kernal_context;
	kernal.mystatus = threadRun;
	
	//struct node node1 ;
	node1.next=NULL;
	node1.thread_pointer = &kernal;
	
	readyQue.head = &node1;
	readyQue.tail = &node1;
	readyQue.que_size = 1;
}

Tid
thread_id()
{
	/* The First Node in the Ready Que is the running one */
	if (readyQue.head == NULL){
		return THREAD_INVALID;	
	}
	
	int valid = get_running_thread_id(&readyQue);
	return valid;
	//TBD();	
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	//printf ("Creating new Thread\n");
	int e_interrupt = interrupts_set(0);
	struct thread *newThread;
	newThread = (struct thread *)malloc(sizeof(thread));
	ucontext_t * newContext = (ucontext_t*)malloc(sizeof(ucontext_t));
	
	if (newThread == NULL || newContext == NULL){
		free(newContext);
		free(newThread);
		interrupts_set(e_interrupt);

		return THREAD_NOMEMORY;
	}
	newThread->mycontext = newContext;
	getcontext(newThread->mycontext);
	
	if (readyQue.que_size < THREAD_MAX_THREADS){
		//newThread->thread_id = get_max_id(readyQue) + 1;
		int count = 0;
		if (readyQue.que_size == 1){
			count = 1;
		}
		else {
			while(count < THREAD_MAX_THREADS){
				if (id_exists(&readyQue, count) == 1){
					count++;
				}
				else{
					break;
				}
			}
		}
		newThread->thread_id = count;
		
		newThread->mystatus = threadReady;
		void * newStack = malloc(2 * THREAD_MIN_STACK);
		void * newStack_ = (void*)(((unsigned long) newStack+8));
		//newStack_ = (void*)((unsigned long)newStack_ - 8);
		
		//void * newStack = malloc(THREAD_MIN_STACK);
		
		if (newStack == NULL){
			free(newStack);
			free(newContext);
			free(newThread);
			interrupts_set(e_interrupt);

			return THREAD_NOMEMORY;
		}
		newThread->stack_p = newStack; // Must free stack_p at allocated
		newContext->uc_stack.ss_sp = newStack_;
		newContext->uc_stack.ss_size = THREAD_MIN_STACK;
		newContext->uc_stack.ss_flags = 0;
		
		unsigned long sp = (unsigned long int)(newContext->uc_stack.ss_sp) + newContext->uc_stack.ss_size;
		
		newContext->uc_mcontext.gregs[REG_RIP] = (long int)thread_stub;
		newContext->uc_mcontext.gregs[REG_RSP] = sp;
		newContext->uc_mcontext.gregs[REG_RSI] = (long int)parg;
		newContext->uc_mcontext.gregs[REG_RDI] = (long int)fn;
				
		add_to_queue(&readyQue, newThread);
		interrupts_set(e_interrupt);

		return newThread->thread_id;
	}
	else {
		free(newContext);
		free(newThread);
		interrupts_set(e_interrupt);
		return THREAD_NOMORE;
	}
	
	//TBD();
	return THREAD_FAILED;
}

Tid
thread_yield(Tid want_tid)						
{
	int e_interrupt = interrupts_set(0);
	if (exitQue.que_size != 0){
		thread_destroy();
	}

	/* Check Status */

	if (want_tid >= THREAD_SELF ){	// Run another thread
		if (want_tid == get_running_thread_id(&readyQue) || want_tid == THREAD_SELF){ // Yield current running thread
			interrupts_set(e_interrupt);
			return get_running_thread_id(&readyQue);
		}
		else if (want_tid == THREAD_ANY){
			//printf("Hello function?\n");
			if (readyQue.que_size == 1){
				interrupts_set(e_interrupt);
				return THREAD_NONE;		// Only one thread and that is the running thread
			}
			thread * current = get_running_thread(&readyQue);
			getcontext(current->mycontext);	//Save key parameters and pointers
			
			//printf("Current Running Thread_ID: %d\n", current->thread_id);
			if (current->mystatus == threadRun){
				current->mystatus = threadReady;
				
				remove_from_queue(&readyQue,0);
				//printf ("Que Size: %d\n", readyQue->que_size);
				add_to_queue(&readyQue, current);
				//printf ("Que Size: %d\n", readyQue->que_size);
				
				thread * next = get_running_thread(&readyQue);	//next thread
				//printf("Next Running Thread_ID: %d\n", next->thread_id);
				//next->mystatus = threadRun;
				setcontext(next->mycontext);	//take out the key parameters and pointers, problems with my setcontext
				interrupts_set(e_interrupt);
				return next->thread_id;
			}
			//else if (current->mystatus == threadExit){
			//	thread_exit();			
			//}
			else {
				current->mystatus = threadRun;
				interrupts_set(e_interrupt);
				return current->thread_id;
			}
			
		}
		else {		// Yield to specific thread_id
			//printf ("Yielding to thread: %d\n", want_tid);
			thread * current = get_running_thread(&readyQue);
			getcontext(current->mycontext);
			
			if (current->mystatus == threadRun){
				int index = 0;
				int found = 0;
				struct node *temp = readyQue.head;
				thread *temp_thread = NULL;
				while (temp != NULL){
					if (temp->thread_pointer->thread_id == want_tid){
						temp_thread = temp->thread_pointer;
						found = 1;
						break;
					}
					index++;
					temp = temp->next;
				}
				if (found == 0){	//Not found
					interrupts_set(e_interrupt);
					return THREAD_INVALID;
				}
				else if (found == 1){
					//printf ("Queue Size: %d\n", readyQue->que_size);

					current->mystatus = threadReady;
					remove_from_queue(&readyQue,0);
					add_to_queue(&readyQue, current);
					move_up(&readyQue, index-1);
					temp_thread->mystatus = threadRun;
					setcontext(temp_thread->mycontext);
					interrupts_set(e_interrupt);
					return want_tid;
				}
			}
			
			else if (current->mystatus == threadExit){
				//printf("Calling threadExit to put into exit queue\n");
				thread_exit();
			}			

			else{
				current->mystatus = threadRun;
				interrupts_set(e_interrupt);
				return want_tid;
			}
		}
	}
	else {	// No need to run anything 
		interrupts_set(e_interrupt);
		return THREAD_INVALID;
	}
	
	//TBD();
	return THREAD_FAILED;
}

Tid
thread_exit()
{
	int e_interrupt = interrupts_set(0);
	//printf("Que Size: %d\n", readyQue->que_size);
	thread * current = get_running_thread(&readyQue);
	//thread * current = readyQue->head->thread_pointer;
	//printf("current ID: %d\n",current->thread_id);
	
	if (readyQue.que_size == 1){
		interrupts_set(e_interrupt);
		return THREAD_NONE;
	}

	if (current->thread_id ==0 && readyQue.que_size == 2){
		remove_from_queue(&readyQue,0);
		current->mystatus = threadExit;
		add_to_queue(&exitQue,current);
		thread * current = get_running_thread(&readyQue); // next thread
		setcontext(current->mycontext); //To Do: Remeber to kill eveything in exitQue!!
		assert(0);
	}
	
	if (current != NULL && current->thread_id!=0)	
	{
		remove_from_queue(&readyQue,0);
		current->mystatus = threadExit;
		add_to_queue(&exitQue,current);
		//thread_yield(THREAD_ANY);

		// Switch to another thread
		
		thread * current = get_running_thread(&readyQue); // next thread
		setcontext(current->mycontext); //To Do: Remeber to kill eveything in exitQue!!
		assert(0);
	}
	interrupts_set(e_interrupt);
	return THREAD_NONE;
}

Tid
thread_kill(Tid tid) /* Only to set the status to exit */
{
	int e_interrupt = interrupts_set(0);
	if (tid <=0 || readyQue.que_size == 0){
		interrupts_set(e_interrupt);
		return THREAD_INVALID;
	}
	else {
		struct node * current = readyQue.head;
		//struct node * prev;
		thread * exitThread = NULL;
		int found = 0;
		while(current !=NULL){
			if (current->thread_pointer->thread_id == tid){
				//printf ("found matching thread_id: %d\n",current->thread_pointer->thread_id);
				exitThread = current->thread_pointer;
				found = 1;
				break;
			}
			current = current->next;
		}
		if (found == 0){
			interrupts_set(e_interrupt);
			return THREAD_INVALID;
		}

		int return_id = exitThread->thread_id;		

		exitThread->mystatus = threadExit;
		
		remove_from_queue(&readyQue,exitThread->thread_id);		//FIX HERE:
		add_to_queue(&exitQue, exitThread);
	
		interrupts_set(e_interrupt);
		return return_id;
	}
	//TBD();
	interrupts_set(e_interrupt);
	return THREAD_FAILED;
}

void
thread_stub(void (*thread_main)(void *), void *arg)
{
	interrupts_set(1);
	Tid ret;
	//printf ("In Stub...\n");
	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
	// all threads are done, so process should exit
	exit(0);
}


void move_up(Queue* Que, int pos){
	struct node * temp = Que->head;
	struct node * prev = NULL;
	int count = 0;
	if (pos == 0){	// Basically only 2 threads in que
		return;
	}
	while(count != pos){
		prev = temp;
		temp = temp->next;
		count ++;
	}
	if (temp == Que->tail){
		Que->tail = prev;
	}
	prev->next = temp->next;
	temp->next = Que->head;
	Que->head = temp;
	return;
}

void thread_destroy(){
	struct node *current = exitQue.head;
	struct node *next;
	while(current!=NULL){
		next = current->next;
		if (current->thread_pointer->thread_id != 0){
			free(current->thread_pointer->stack_p);
			free(current->thread_pointer->mycontext);
			free(current->thread_pointer);
			free(current);
		}
		current = next;
	}
	exitQue.head = NULL;
	exitQue.tail = NULL;
	exitQue.que_size = 0;
	
}

void add_to_queue(Queue *Que, thread *thread_){
	/* Create new node */
	if (thread_->thread_id != 0){
		struct node * newNode = (struct node *)malloc(sizeof(node));
		newNode->thread_pointer = thread_;
		if (Que->que_size == 0){ //empty queue
			Que->head = newNode;
			Que->tail = newNode;
			newNode->next = NULL;
			Que->que_size++;
			return;
		}
		else if (Que->que_size == 1){
			if (Que->head == NULL){
				Que->head = Que->tail;
			}
			Que->tail->next = newNode;
			Que->tail = newNode;
			Que->head->next = Que->tail;
			newNode->next = NULL;
			Que->que_size++;
			return;
		}
		else { //Add to end of the que
			Que->tail->next = newNode;
			Que->tail = newNode;
			newNode->next = NULL;
			Que->que_size++;
			return;
		}
	}
	else{
		node1.thread_pointer = thread_;
		if (Que->que_size == 0){ //empty queue
			Que->head = &node1;
			Que->tail = &node1;
			node.next = NULL;
			Que->que_size++;
			return;
		}
		
		else if (Que->que_size == 1){
			if (Que->head == NULL){
				Que->head = Que->tail;
			}
			Que->tail->next = &node1;
			Que->tail = &node1;
			Que->head->next = Que->tail;
			node1.next = NULL;
			Que->que_size++;
			return;
		}
		else { //Add to end of the que
			Que->tail->next = &node1;
			Que->tail = &node1;
			node1.next = NULL;
			Que->que_size++;
			return;
		}
	}
	
} 

int get_running_thread_id(Queue *Que){
	if (Que->head != NULL){
		return Que->head->thread_pointer->thread_id;
	}
	return Que->tail->thread_pointer->thread_id;
}

int id_exists(Queue *Que, int count){
	struct node *temp = Que->head;
	while(temp!=NULL){
		if(temp->thread_pointer->thread_id == count){
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

thread *get_running_thread(Queue *Que){
	if (Que->head != NULL){
		return Que->head->thread_pointer;
	}
	return Que->tail->thread_pointer;
	
}

void remove_from_queue (Queue *Que, int n){
	if (Que->que_size == 1){
		free(Que->head);
		Que->head = NULL;
		Que->tail = NULL;
		Que->que_size = 0;
		return;
	}
	else if (n == 0){
		struct node * temp = Que->head;
		Que->head = Que->head->next;
		if (temp->thread_pointer->thread_id != 0){
			free(temp);
		}
		Que->que_size--;
		return;
	}
	else{
		struct node *temp = Que->head;
		struct node *prev = NULL;
		while(temp !=NULL){
			if (temp->thread_pointer->thread_id == n){
				if (temp->next != NULL){
					prev->next = temp->next;				
				}
				else{
					Que->tail = prev;
					prev->next = NULL;				
				}				
				
				if (temp->thread_pointer->thread_id != 0){
					free(temp);
				}
				Que->que_size--;
				return;
			}
			prev = temp;
			temp = temp->next;		
		}
		
	}
	return;
}
/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
	Queue *waitQue;
}wait_queue;

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	wq->waitQue = (Queue*)malloc(sizeof(struct Queue));
	wq->waitQue->head = NULL;
	wq->waitQue->tail = NULL;
	wq->waitQue->que_size = 0;
	
	assert(wq);

	//TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	struct node * current = wq->waitQue->head;
	struct node * next = NULL;
	while(current != NULL){
		next = current->next;
		if (current->thread_pointer->thread_id != 0){
			free(current->thread_pointer->stack_p);
			free(current->thread_pointer->mycontext);
			free(current->thread_pointer);
			free(current);
		}
		current = next;
	}
	//TBD();
	free(wq->waitQue);
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	//TBD();
	int e_interrupt = interrupts_set(0);	// Disable interrupts
	
	if (queue == NULL){
		interrupts_set(e_interrupt);
		return THREAD_INVALID;
	}
	
	if (readyQue.que_size == 1){
		interrupts_set(e_interrupt);

		return THREAD_NONE;
	}
	
	thread * current = get_running_thread(&readyQue);
	getcontext(current->mycontext);
	
	if (current->mystatus == threadRun || current->mystatus == threadReady){
		current->mystatus = threadSleep;
		remove_from_queue(&readyQue, 0);
		add_to_queue(queue->waitQue, current);		//Specify which waitQue it is		
		thread * next = get_running_thread(&readyQue);
		setcontext(next->mycontext);
	}
	
	else {
		current->mystatus = threadRun;		
	}
	
	interrupts_set(e_interrupt);
	return get_running_thread_id(&readyQue);

	
	//return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	int e_interrupt = interrupts_set(0);
	if (queue == NULL || queue->waitQue->que_size == 0){
		interrupts_set(e_interrupt);
		return 0;
	}

	if (all == 0){
		thread * current = queue->waitQue->head->thread_pointer; 
		current->mystatus = threadAwake;
		remove_from_queue(queue->waitQue,0);
		add_to_queue(&readyQue, current);
		interrupts_set(e_interrupt);
		return 1;
		
	}
	else if (all == 1){		//Wake up Everythread
		int count = 0;
		while(queue->waitQue->head != NULL){
			thread * current = queue->waitQue->head->thread_pointer;
			current->mystatus = threadAwake;
			remove_from_queue(queue->waitQue,0);
			add_to_queue(&readyQue, current);
			count++;
		}
		interrupts_set(e_interrupt);

		return count;
	}
	
	//TBD();
	return 0;
}

/*typedef enum lock_status{
	lock_free,
	lock_taken
} lock_status;*/

struct lock {
	/* ... Fill this in ... */
	//lock_status status;

	int taken;	
	
	thread * has_lock;	//
	struct wait_queue * lock_que;
}lock;

struct lock *
lock_create()
{
	int e_interrupt = interrupts_set(0);
	struct lock *lock = malloc(sizeof(struct lock));
	lock->taken = 0;		// Initally lock staus is free
	lock->has_lock = NULL;		// Initally, no one has the lock
	lock->lock_que = wait_queue_create();
	
	assert(lock);

	//TBD();
	
	interrupts_set(e_interrupt);
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	int e_interrupt = interrupts_set(0);
	assert(lock != NULL);
	
	wait_queue_destroy(lock->lock_que);
	
	
	//TBD();
	interrupts_set(e_interrupt);

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	int e_interrupt = interrupts_set(0);
	

	if (lock->has_lock == get_running_thread(&readyQue)){
		interrupts_set(e_interrupt);
		return;
	}	

	
	while(lock->taken == 1 && lock->has_lock != get_running_thread(&readyQue)){
		thread_sleep(lock->lock_que);		// Block all other threads
	}
	

	assert(lock != NULL);
	lock->taken = 1;
	lock->has_lock = get_running_thread(&readyQue);
	//TBD();
	interrupts_set(e_interrupt);

}

void
lock_release(struct lock *lock)
{
	int e_interrupt = interrupts_set(0);
	
	if (lock->has_lock != NULL){
		lock->taken = 0;
		lock->has_lock = NULL;
		thread_wakeup(lock->lock_que,1);		//Wake up all threads
	}
	
	assert(lock != NULL);
	interrupts_set(e_interrupt);
	//TBD();
}

struct cv {
	/* ... Fill this in ... */
	struct wait_queue* cv_que;
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	cv->cv_que = wait_queue_create();
	
	//TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	//TBD();
	if (cv != NULL){
		wait_queue_destroy(cv->cv_que);
		free(cv);
	}
	
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int e_interrupt = interrupts_set(0);
	
	assert(cv != NULL);
	assert(lock != NULL);
	
	if(lock->has_lock == get_running_thread(&readyQue)){
		lock_release(lock);
		thread_sleep(cv->cv_que);
		lock_acquire(lock);
	}
	interrupts_set(e_interrupt);
	
	//TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int e_interrupt = interrupts_set(0);
	
	assert(cv != NULL);
	assert(lock != NULL);

	if (get_running_thread(&readyQue) == lock->has_lock){	// Check if calling thread acquired the lock, release lock right after signaling
		thread_wakeup(cv->cv_que,0);				// Wake up one thread
	}
	interrupts_set(e_interrupt);
	
	//TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int e_interrupt = interrupts_set(0);
	
	
	
	assert(cv != NULL);
	assert(lock != NULL);

	if (get_running_thread(&readyQue) == lock->has_lock){	// Check if calling thread acquired the lock
		thread_wakeup(cv->cv_que,1);				// Wake up everybody
	}
	interrupts_set(e_interrupt);
	//TBD();
}
