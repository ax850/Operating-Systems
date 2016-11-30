#include "request.h"
#include "server_thread.h"
#include "common.h"
#include "string.h"
typedef struct node {
	struct node * next;		// Points to next node (Thread)
	struct node * prev;
	int connfd_;	// Points to the Thread
	void * pointer; // Points to cache block
}node;

typedef struct Queue {	/* Creating Linked List */
	
	struct node * head;
	struct node * tail;	
	int que_size;
	
}Queue;



typedef pthread_mutex_t mutex;	//mutex lock
typedef pthread_cond_t cond;	//condition

/* Cache Functions and Declaration */

typedef struct c_block{
	int check;
	node * block;	/*Want to see who is before and after the cache block*/
	struct file_data * data;
}c_block;

typedef struct hash_node{
	 c_block* block;
	struct hash_node *next;
	struct hash_node *prev;
	char *filename;
}hash_node;

typedef struct hash_t{
	hash_node *head[10000];
}hash_t;



typedef struct cache{
	hash_t * c_table;
	Queue* c_que;		// List of caches
	int total_size;
	int curr_size;
	int check;
	mutex c_lock;

}cache;

hash_t * new_hash();
void * hash_find(hash_t *hash_table, char *key);
int hash_insert(hash_t *hash_table, char * key, void* block);
void hash_delete(hash_t * hash_table, char * key);
int are_equal(char *s1,char *s2);
void hash_delete(hash_t * hash_table, char * key);


void * que_tail(Queue * Que);
void add_to_queue(Queue * Que, int *connfd);
void pop_from_queue(Queue * Que);
void remove_from_list(Queue * Que);
void move_top(Queue * Que, node * temp);
void que_prepend(Queue * Que, c_block* block);

int cache_evict(int evict_size, cache * cache){
	if(evict_size > cache->total_size){		// Too big of a file, evict_size is file_size
		return 0;
	}
	if(cache->curr_size == 0 || cache->c_que->que_size == 0){
		return 0;
	}
	else{
		while((cache->total_size - cache->curr_size) < evict_size){
			c_block * block_ = (c_block *)que_tail(cache->c_que);
			if(block_->check > 0 ){	//if block is being sent
				//printf("Can't evict, cache block being used...\n");
				move_top(cache->c_que,block_->block);
				if(cache->check == cache->c_que->que_size){
					return 0;
				}
			}
			else{

				struct file_data * file = block_->data;
				//evict_size = evict_size - file->file_size;
				cache->curr_size = cache->curr_size - file->file_size;
				remove_from_list(cache->c_que);		// Remove last from list
				hash_delete(cache->c_table,file->file_name);						//Remove data from cache, implement this later
			}			
		}
		return 1;
	}
	
}


cache* cache_init(int max_size){
	cache* new_cache = (cache*)malloc(sizeof(cache));
	pthread_mutex_init(&(new_cache->c_lock), NULL);
	new_cache->c_table = new_hash();		// new hash to store cache blocks
	new_cache->c_que = (Queue*)malloc(sizeof(Queue));			// list to determine order of cache blocks using LRU
	new_cache->c_que->head=NULL;
	new_cache->c_que->tail=NULL;
	new_cache->c_que->que_size=0;
	new_cache->check = 0;
	
	new_cache->total_size = max_size;
	new_cache->curr_size = 0;
	
	return new_cache;
}

int cache_lookup(cache * cache_,struct file_data* file ){
	

	pthread_mutex_lock(&(cache_->c_lock));		//lock
	if(cache_ == NULL || file == NULL){
		pthread_mutex_unlock(&(cache_->c_lock));
		return 0;
	}
	
	if(file->file_size > cache_->total_size){	//file size greater than cache?
		pthread_mutex_unlock(&(cache_->c_lock));
		return 0;
	}
	
	else{
		void * ret= hash_find(cache_->c_table,file->file_name);		//implement this later
		//void * ret = NULL;
		if(ret == NULL){		//not found
			pthread_mutex_unlock(&(cache_->c_lock));
			return 0;
		}
		else{
			c_block * temp_blk = (c_block*)ret;
			
			if(temp_blk->check == 0){
				cache_->check++;
			}
			temp_blk->check++;
			struct node * temp1 = temp_blk->block;		//block of cache stored in temp1
			//printf("Found and moving to top\n");
			move_top(cache_->c_que, temp1);
			file->file_buf = temp_blk->data->file_buf;
			file->file_size = temp_blk->data->file_size;
			pthread_mutex_unlock(&(cache_->c_lock));
			return 1;
		}
	}
	
	
}

int cache_insert(cache * cache_, struct file_data * file){
	pthread_mutex_lock(&(cache_->c_lock));
	if(cache_ == NULL || file == NULL){
		pthread_mutex_unlock(&(cache_->c_lock));
		return 0;
	}
	
	if(file->file_size >= cache_->total_size ){	//file size greater than cache?
		pthread_mutex_unlock(&(cache_->c_lock));
		return 0;
	}
	
	void * ret = hash_find(cache_->c_table, file->file_name);
	//void * ret = NULL;
	if(ret == NULL){		//Doesn't exist in cache
		//printf("Doesn't exist in cache\n");
		if((cache_->total_size - cache_->curr_size) < file->file_size ){
			//printf("Need Evict, Current size: %d and data file size:%d \n", cache_->curr_size,file->file_size);
			int ret2 = cache_evict(file->file_size, cache_);
			if(ret2 == 0){
				pthread_mutex_unlock(&(cache_->c_lock));
				return 0;
			}
		}
		c_block * new_block = (c_block*)malloc(sizeof(c_block));
		new_block->data = file;
		new_block->check = 0;
		hash_insert(cache_->c_table,file->file_name, new_block);
		que_prepend(cache_->c_que, new_block);
		cache_->curr_size +=file->file_size;
		//printf("Done Inserting: %s\n",file->file_name);
		new_block->block = cache_->c_que->head;			//c_que is a list of node type things
		pthread_mutex_unlock(&(cache_->c_lock));
		return 1;
		
	}
	else{
		c_block * temp = (c_block*)ret;
		temp->check--;
		if(temp->check == 0){
			cache_->check--;
		}
		pthread_mutex_unlock(&(cache_->c_lock));
		return 1;
	}
		
}


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
	//printf("\n\nLooking up...%s\n",data->file_name);
	int ret2 = cache_lookup(sv->cache_,data);
	if(ret2!=0){
		//printf("Cache found\n");
		request_sendfile(rq);
	}
	else{
		//printf("Cached miss\n");
        ret = request_readfile(rq);
        if(!ret){
            // printf("Read file invalid\n");
          request_destroy(rq);
          file_data_free(data);
          return;
        }
        else{
            //printf("File read, sending, size : %d", data->file_size);
            request_sendfile(rq);
        }
		//printf("Cache Insert\n");
		cache_insert(sv->cache_,data);
    }
	
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
		sv->cache_= cache_init(max_cache_size);
		pthread_mutex_init(sv->lock, NULL);
		pthread_cond_init(sv->full_pool,NULL);
		pthread_cond_init(sv->empty_pool,NULL);
		
		/* Create Worker Threads */
		
		//pthread_t * threadList = (struct pthread_t*)malloc(sizeof(nr_threads*pthread_t));
		
		pthread_t threadList[nr_threads];
		
		int i = 0;
		
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

void *stub(void * sv_){	//Implement 
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

void * que_tail(Queue * Que){
	if(Que == NULL || Que->tail == NULL){
		return NULL;
	}
	else{
		return Que->tail->pointer;
	}
}

hash_t * new_hash(){
	struct hash_t* hash_t;
	hash_t = (struct hash_t*)malloc(sizeof(struct hash_t));
	int j = 0;
	while(j<10000){
		hash_t->head[j] = NULL;
		j++;
	}
	return hash_t;
}

unsigned long djb2hash(char *str){
	unsigned long hash_index = 5381;
	int c=0;
	
	while((c = *str++)){
		hash_index = ((hash_index << 5) + hash_index) + c;
	}
	return hash_index;
	
}	

int hash_insert(hash_t *hash_table, char * key, void* block){

	unsigned long index = djb2hash(key)%10000;
	//struct hash_node *pointer = hash_table->head[index];
	
	if(hash_table->head[index] == NULL){
		struct hash_node * new = (struct hash_node*)malloc(sizeof(hash_node));
		new->block = (c_block*) block;
		new->filename = key;
		new->next = NULL;
		new->prev = NULL;
		hash_table->head[index] = new;
	}
	else{
		hash_node *pointer = hash_table->head[index];
		while(pointer->next !=NULL){
			if (pointer->filename == key){	//Error here as well
				return 1;
			}
			pointer = pointer ->next;
		}
		if(pointer->filename == key){
			return 1;
		}
		else {
			struct hash_node * new = (struct hash_node*)malloc(sizeof(hash_node));
			new->block = (c_block*)block;
			new->filename = key;
			new->next = NULL;
			pointer->next = new;
			new->prev = pointer;
			return 1;
		}
	}
	return 1;
}

void * hash_find(hash_t *hash_table, char *key){
	unsigned long index = djb2hash(key)%10000;
	hash_node *pointer = hash_table->head[index];
	//printf("index:%lu and the key is %s\n", index, key);
	while(pointer != NULL){
		/*if(pointer->filename == key){ //Sometime wrong with this part..., filename and key aren't same value
			return pointer;
		}*/
		
		if(are_equal(key,pointer->filename)){
			//printf("File found\n");
			return pointer->block;
		}
		pointer = pointer->next;
	}
	return NULL;
}

void * hash_find_del(hash_t *hash_table, char *key){
	unsigned long index = djb2hash(key)%10000;
	hash_node *pointer = hash_table->head[index];
	//printf("index:%lu and the key is %s\n", index, key);
	while(pointer != NULL){
		/*if(pointer->filename == key){ //Sometime wrong with this part..., filename and key aren't same value
			return pointer;
		}*/
		
		if(are_equal(key,pointer->filename)){
			//printf("File found\n");
			return pointer;
		}
		pointer = pointer->next;
	}
	return NULL;
}

void hash_delete(hash_t * hash_table, char * key){

	void * ret = hash_find_del(hash_table, key);
	unsigned long index = djb2hash((char*)key)%10000;
	if(ret != NULL){
		struct hash_node * temp = (hash_node*)ret;
		
		if(temp->prev == NULL){
			hash_table->head[index] = temp->next;
		}
		else if(temp->next ==NULL){
			temp->prev->next = NULL;
		}
		else{
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
		}
		file_data_free(temp->block->data);
		//free(temp->block->data);
		free(temp->block);
		free(temp);
		return;
	}
	else{
		fprintf(stderr, "WHAT? It's not in the hash_table...\n");
		return;
	}
}

int are_equal(char *s1, char *s2){
	if (strlen(s1) != strlen(s2))
        return 0; // They must be different
    int i = 0;
	for (i = 0; i < strlen(s1); i++)
    {
        if (s1[i] != s2[i])
            return 0;  // They are different
    }
    return 1;  // They must be the same
}
