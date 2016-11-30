#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

struct queue {
	struct node * head;
	int size;
};

struct node{
	struct node * next;
};

typedef enum thread_status {
	running,
	ready,
	exit,
	blocked
} thread_status;

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	int thread_id;
	thread_status my_thread_status;
	ucontext_t *my_context; 
	
};

void
thread_init(void)
{
	/* your optional code here */
	thread_ready_queue = (struct queue *)malloc(sizeof(struct queue));
	thread_ready_queue->head=NULL;
	thread_ready_queue->size = 0;
	thread_exit_queue = (struct queue *)malloc(sizeof(struct queue));
	thread_exit_queue->head=NULL;
	thread_exit_queue->size = 0;
	
	/* Create First Custom Thread*/
	
	thread * newThread = (struct thread *)malloc(sizeof(struct thread));
	
	ucontext_t * newContext = (ucontext_t)malloc(sizeof(ucontext_t));
	newThread->thread_id=0;
	newThread->my_context = newContext
}

Tid
thread_id()
{
	TBD();
	return THREAD_INVALID;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	TBD();
	return THREAD_FAILED;
}

Tid
thread_yield(Tid want_tid)
{
	TBD();
	return THREAD_FAILED;
}

Tid
thread_exit()
{
	TBD();
	return THREAD_FAILED;
}

Tid
thread_kill(Tid tid)
{
	TBD();
	return THREAD_FAILED;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
};

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
