#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>

#define block_size 4096 //4KB

typedef pthread_mutex_t mutex;	//mutex lock
typedef pthread_cond_t cond;	//condition

/*Filename is key*/


typedef struct Queue {	/* Creating Linked List */
	
	struct node * head;
	struct node * tail;	
	int que_size;
	
}Queue;

typedef struct node {
	struct node * next;		// Points to next node (Thread)
	struct node * prev;
	int connfd_;	// Points to the Thread
	void * pointer;
}node;

typedef struct c_block{
	int position;
	node * block;	/*Want to see who is before and after the cache block*/
	struct file_data * data;
}c_block;


typedef struct cache{
	//hash_t * c_table;
	Queue* c_que;		// List of caches
	int total_size;
	int curr_size;
	int position;
	mutex *c_lock;

}cache;

void pop_from_queue(Queue * Que);
void add_to_queue(Queue * Que, int *connfd);
void remove_from_list(Queue * Que);
void move_top(Queue * Que, node * temp);
void que_prepend(Queue * Que, c_block* block);

void *hash_find();

/* Cache Stuff */


cache* cache_init(int max_size){
	cache* new_cache = (struct cache*)malloc(sizeof(cache));
	pthread_mutex_init(new_cache->c_lock, NULL);
	//new_cache->c_table = new_hash();		// new hash to store cache blocks
	new_cache->c_que = (struct Queue*)malloc(sizeof(Queue));			// list to determine order of cache blocks using LRU
	new_cache->c_que->head=NULL;
	new_cache->c_que->tail=NULL;
	new_cache->c_que->que_size=0;
	
	
	new_cache->total_size = max_size;
	new_cache->curr_size = 0;
	
	return new_cache;
}


int cache_evict(int evict_size, cache * cache){
	if(evict_size > cache->total_size){		// Too big of a file
		return 0;
	}
	else{
		while(evict_size > 0){
			c_block * block = cache->c_que->tail->pointer;
			/*if(cache->c_que->que_size!=0){
				c_block * block = cache->c_que->tail->pointer;		//Least Recently used
			}*/
			struct file_data * file = block->data;
			evict_size = evict_size - file->file_size;
			cache->curr_size = cache->curr_size - file->file_size;
			remove_from_list(cache->c_que);		// Remove last from list
			//hash_delete;						//Remove data from cache, implement this later
		}
		return 1;
	}
	
}


int cache_lookup(cache * cache_,struct file_data* file ){
	pthread_mutex_lock(cache_->c_lock);		//lock
	if(cache_ == NULL || file == NULL){
		pthread_mutex_unlock(cache_->c_lock);
		return 0;
	}
	
	if(file->file_size > cache_->total_size){	//file size greater than cache?
		pthread_mutex_unlock(cache_->c_lock);
		return 0;
	}
	
	else{
		void * ret= hash_find();		//implement this later
		if(ret == NULL){		//not found
			pthread_mutex_unlock(cache_->c_lock);
			return 0;
		}
		else{
			c_block * temp_blk = (c_block*)ret;
			struct node * temp1 = temp_blk->block;		//block of cache stored in temp1
			move_top(cache_->c_que, temp1);
			file->file_buf = temp_blk->data->file_buf;
			file->file_size = temp_blk->data->file_size;
			pthread_mutex_unlock(cache_->c_lock);
			return 1;
		}
	}
	
	
}

int cache_insert(cache * cache_, struct file_data * file){
	pthread_mutex_lock(cache_->c_lock);
	if(cache_ == NULL || file == NULL){
		pthread_mutex_unlock(cache_->c_lock);
		return 0;
	}
	
	if(file->file_size > cache_->total_size){	//file size greater than cache?
		pthread_mutex_unlock(cache_->c_lock);
		return 0;
	}
	
	void * ret = hash_find();
	if(ret == NULL){		//Doesn't exist in cache
		if((cache_->total_size - cache_->curr_size) < file->file_size ){
			printf("Need Evict\n");
			int ret2 = cache_evict(file->file_size, cache_);
			if(ret2 == 0){
				pthread_mutex_unlock(cache_->c_lock);
				return 0;
			}
		}
		c_block * new_block = (struct c_block*)malloc(sizeof(c_block));
		new_block->data = file;
		new_block->position = 0;
		//hash_insert(_cache->c_table,file->file_name, new_block);
		que_prepend(cache_->c_que, new_block);
		cache_->curr_size +=file->file_size;
		
		new_block->block = cache_->c_que->head;			//c_que is a list of node type things
		pthread_mutex_unlock(cache_->c_lock);
		return 1;
		
	}
	pthread_mutex_unlock(cache_->c_lock);
	return 1;
		
}

/* Server Stuff*/




struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	/* add any other parameters you need */
	
	mutex * lock;
	cond * full_pool;		// If que is full then we must wait...
	cond * empty_pool;		// If que is empty, then worker must wait...
	Queue *request_que;
	cache * cache_;
};

/* Que Function Declaration */


void add_to_queue(Queue * Que, int *connfd);
void pop_from_queue(Queue * Que);
void * stub(void * sv_);
/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fills data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	
	/* Look up Cache */
	int ret2 = cache_lookup(sv->cache_,data);
	if (ret2!=0){
		request_sendfile(rq);
	}
	else{
		/* reads file, 
		 * fills data->file_buf with the file contents,
		 * data->file_size with file size. */
		ret = request_readfile(rq);
		if (!ret)
			goto out;
		/* sends file to client */
		request_sendfile(rq);
	out:
		request_destroy(rq);
		file_data_free(data);
	}
	cache_insert(sv->cache_,data);
	request_destroy(rq);
}

/* entry point functions */

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;

	if (nr_threads > 0 || max_requests > 0 || max_cache_size >= 0) {	// Cache is 0 for Lab4
		
		/* Initialize Que */
		
		sv->request_que = (Queue *)malloc(sizeof(Queue));
		sv->request_que->head = NULL;
		sv->request_que->tail = NULL;
		sv->request_que->que_size = 0;
		
		/* Initialize Locks and Conditional Variables */
		
		sv->lock = (mutex*)malloc(sizeof(mutex));
		sv->full_pool = (cond*)malloc(sizeof(cond));
		sv->empty_pool = (cond*)malloc(sizeof(cond));
		sv->cache_= cache_init(max_cache_size);		//Initialize Cache
		pthread_mutex_init(sv->lock, NULL);
		pthread_cond_init(sv->full_pool,NULL);
		pthread_cond_init(sv->empty_pool,NULL);
		
		/* Create Worker Threads */
		
		//pthread_t * threadList = (struct pthread_t*)malloc(sizeof(nr_threads*pthread_t));
		
		pthread_t threadList[nr_threads];
		
		int i = 0;
		if(max_cache_size>0)
		{
			while(i < nr_threads){
			pthread_create(&threadList[i], NULL, stub, (void *)sv);		//seperate stub for cache
			i++;
		}
		}
		
		while(i < nr_threads){
			pthread_create(&threadList[i], NULL, stub, (void *)sv);
			i++;
		}
		
		//TBD();
	}

	/* Lab 4: create queue of max_request size when max_requests > 0 */

	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

	return sv;
}


void *stub(void * sv_){	//Implement Later
	

	struct server* sv = (struct server*) sv_;
	
	start:
	
	pthread_mutex_lock(sv->lock);
	
	while(sv->request_que->que_size == 0){
		pthread_cond_wait(sv->empty_pool, sv->lock);	//Pool empty, worker threads must wait
	}
	
	int connfd = sv->request_que->head->connfd_;
	
	
	pop_from_queue(sv->request_que);		// Pool shouldnt be full now
	
	pthread_cond_signal(sv->full_pool);	//Wake up since not full
	pthread_mutex_unlock(sv->lock);
	do_server_request(sv, connfd);
	goto start;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		//TBD();
		
		pthread_mutex_lock(sv->lock);	//Acquire lock
		while(sv->request_que->que_size == sv->max_requests){	//Buffer Full
			pthread_cond_wait(sv->full_pool, sv->lock);	
			
		}
		
		add_to_queue(sv->request_que, &connfd);	//Not full anymore
		
		if (sv->request_que->que_size > 0){	// Signal worker thread to do some work
			pthread_cond_signal(sv->empty_pool);	// No longer empty, so wake up!
		}
		
		pthread_mutex_unlock(sv->lock);
	}
}

/* Queue Functions */

void pop_from_queue(Queue * Que){
	if (Que == NULL){
		return;
	}
	if(Que->que_size == 1){
		free(Que->head);
		Que->head = NULL;
		Que->tail = NULL;
		Que->que_size = 0;
		return;
	}
	else {
		struct node * temp = Que->head;
		Que->head = Que->head->next;
		Que->head->prev = NULL;
		free(temp);
		Que->que_size--;
		return;
	}
}

void add_to_queue(Queue * Que, int *connfd){
	
	struct node * newNode = (struct node *)malloc(sizeof(node));
	newNode->connfd_ = *connfd;
	
	if (Que->que_size == 0){
		Que->head = newNode;
		Que->tail = newNode;
		Que->tail->next = NULL;
		Que->tail->prev = NULL;
		Que->que_size++;
		return;
		
	}
	else {
		newNode->prev = Que->tail;
		Que->tail->next = newNode;
		Que->tail =  newNode;
		Que->tail->next = NULL;
		Que->que_size++;
		return;
		
	}
	
	
}

void remove_from_list(Queue * Que){
	if (Que == NULL || Que->que_size == 0){
		return;
	}
	
	if(Que->que_size == 1){
		free(Que->head);
		Que->head=NULL;
		Que->tail=NULL;
		Que->que_size = 0;
		return;
	}
	else{
		struct node * temp;
		temp = Que->tail;
		Que->tail = Que->tail->prev;
		Que->tail->next = NULL;
		free(temp);
		Que->que_size = Que->que_size - 1;
		return;
	}
	
}

void move_top(Queue * Que, node * temp){
	if(Que == NULL|| temp == NULL){
		return;
	}
	else{
		if(Que->que_size == 1 || (Que->head == temp)){
			return;
		}
		else if(Que->tail == temp){
			temp->prev->next = NULL;
			Que->tail = temp->prev;
		}
		else{
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		temp->prev = NULL;
		temp->next = Que->head;
		Que->head->prev = temp;
		Que->head = temp;
		return;
		
	}
}

void que_prepend(Queue * Que, c_block* block){
	struct node * temp = (struct node *)malloc(sizeof(node));
	if(temp == NULL){
		return;
	}
	temp->pointer = block;
	if(Que->que_size == 0){
		temp->next = NULL;
		temp->prev = NULL;
		Que->head = temp;
		Que->tail = temp;
		Que->que_size = 1;
		return;

	}
	else{
		temp->next = Que->head;
		Que->head->prev = temp;
		Que->head = temp;
		Que->head->prev = NULL;
		Que->que_size +=1;
		return;
	}
}


void * hash_find(){
	return NULL;
}

