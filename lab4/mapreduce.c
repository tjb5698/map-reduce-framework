/******************************************************************************
 * Your implementation of the MapReduce framework API.
 *
 * Other than this comment, you are free to modify this file as much as you
 * want, as long as you implement the API specified in the header file.
 *
 * Note: where the specification talks about the "caller", this is the program
 * which is not your code.  If the caller is required to do something, that
 * means your code may assume it has been done.
 ******************************************************************************/

#include "mapreduce.h"

#define SUCCESS   1
#define FAILURE  -1

#define NO_CLAIM -1
#define LOCKED    true

void * helloWorld( void * thread_id );

struct map_reduce *mr_create(map_fn map, reduce_fn reduce, int threads, int buffer_size) {
	
	/* invalid map function */
	//if (map == NULL)
	//	return NULL;

	/* invalid reduce function */
	//if (reduce == NULL)
	//	return NULL;

	/* invalid thread count */
	if (threads < 1)
		return NULL;

	/* invalid buffer size */
	if (buffer_size < 1)
		return NULL;

	/* create a new map_reduce struct */
  	struct map_reduce * mr = malloc(sizeof(struct map_reduce));

	/* malloc failed */
	if (mr == NULL)
		return NULL;

	/* initialize the initial values  */
	mr->map          = map;
	mr->reduce       = reduce;
	mr->threads      = threads;
	mr->buffer_size  = buffer_size;

	/* set up thread pointer block */
	mr->mapThreads   = malloc(threads * sizeof(pthread_t));
	mr->reduceThread = malloc(sizeof(pthread_t));

	/* set up the shared lockers (buffer) */
	mr->locker_count = buffer_size/sizeof(struct kvpair);
	mr->lockers      = malloc(mr->locker_count);
	// mr->lockers   = malloc(mr->locker_count * sizeof(struct kvpair));
	mr->locks 	 	 = malloc(mr->locker_count * sizeof(bool));
	mr->claims       = malloc(mr->threads * sizeof(int));

	for (int i = 0; i < mr->locker_count; i++)
		(mr->locks)[i] = !(LOCKED);

	for (int j = 0; j < mr->threads; j++)
		// no mapper thread has claimed a locker
		(mr->claims)[j] = NO_CLAIM;

	mr->lockers_in_use = 0;

	/* allocate space for and initialize the condition
	   variable and its corresponding mutex variable   */
	mr->lock_available_mutex      = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	mr->mrop_mutex                = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	mr->lock_available_condition  = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	mr->lock_empty_condition      = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	mr->mrop_complete_condition   = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

	mr->map_status = 0;

	//mr->buffer_space = buffer_size; 		Keeps track of space left in buffer
	//mr->empty = 0				;	signal from consumer to producer if buffer is empty
	//mr->full = numBuffer			;	signal from producer to consumer if buffer is full
    	//mr->bs_mutex				;	lock for buffer space
	//mr->Buff = malloc(Buff sizeof(buffer));	creating a buffer

	/* return a pointer to the new map_reduce struct */
	return mr;
}

void mr_destroy(struct map_reduce *mr) {
	if (mr != NULL)
	{
		free(mr);
	}
}

void mr_start_helper(struct map_reduce *mr, int infd, int thread_id, int *retval)
{
	// the actual function call to map()
	retval = &(mr->map(mr, infd, thread_id, mr->threads));	
}

void mr_reduce_helper(struct map_reduce *mr, int outfd, int *retval, int *reduce_count)
{
	// the actual function call to reduce()
	retval = &(mr->reduce(mr, outfd, mr->threads));
	if (retval == 0)
		*reduce_count = + 1;
	if (*reduce_count == mr->threads)
		mr->map_status = 0;
	pthread_cond_signal(& (mr->mrop_complete_condition));	//signal that done 

}

int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath) {
	int * retval;	// shared integer to capture return values
	int * reduce_count = 0;
	int * infd;
	int * outfd;

	for (int thread_id = 0; thread_id < mr->threads; thread_id++)
	{
		// get a pointer to the thread
		pthread_t * mapThread = & ( (mr->mapThreads)[thread_id] );

		// todo: create inpath & outpath file descriptos
		// see file i/o code from earlier :)
		*infd = open(inpath, O_RDONLY | O_CREAT);
		if (*infd == FAILURE){
			printf("File doesn't exist.\n");
			return FAILURE;
		}
		
		// create the next map thread
		/* map(mr, infd, id, nmaps): map function */

		pthread_create(mapThread,		/* the thread */
					NULL,				/* attributes */
					mr_start_helper,
			  	(void *) mr,			/*    arg0    */
			  	(void *)	infd, 		/*    arg1    */
			 	(void *)	&thread_id, /*    arg2    */
			  	(void *)	retval);	/*    arg3    */

		// stop if thread creation not successful
		if (*retval != 0){
			mr->map_status = -1;
			return FAILURE;
		}
	}

	*outfd = open(outpath, O_APPEND | O_CREAT);
	if (*outfd == FAILURE){
			printf("File doesn't exist.\n");
			return FAILURE;
		}
	
	// create the reduce thread
	pthread_t * reduceThread = mr->reduceThread;

	pthread_create(reduceThread,
					NULL,
					mr_reduce_helper,
					(void *) mr,
					(void *) outfd,
					(void *) retval,
					(void *) reduce_count);

	if (*retval != 0){
		mr->map_status = -1;
		return FAILURE;
	}
	return SUCCESS;
}

/* test function */
/*void *helloWorld( void * thread_id )
{
	long tid = (long) thread_id;
	printf("Hello from thread %ld!\n", tid);
	pthread_exit(NULL);
}*/

int mr_finish(struct map_reduce *mr) {
	if((mr->map_status) == -1)
		return FAILURE;

	while ((mr->map_status) != 0)
		pthread_cond_wait( &(mr->mrop_complete_condition), & (mr->mrop_mutex) );

	for (int i = 0; i < (mr->threads); i++){
		int close_file = close(i);
	}
	return 0;
}

int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv) {

	// check if buffer is too small to hold even a single kvpair
	if ((mr->buffer_size) < sizeof(struct kvpair))
		return FAILURE;

	// mutex lock??

	// if all the lockers are in use, wait until a locker is available
	while (mr->lockers_in_use == mr->locker_count)
	{
		pthread_cond_wait( &(mr->lock_available_condition), & (mr->lock_available_mutex) );
	}

	// a locker is available
	pthread_mutex_lock(& (mr->lock_available_mutex) );

	// search for the empty locker
	for (int i = 0; i < (mr->locker_count); i++)
	{
		if (mr->locks[i] != LOCKED)
		{
			// we found an unlocked locker!
			// "mine mine mine" (Finding Nemo reference)
			mr->claims[id] = i;
			mr->locks[i]   = LOCKED;
			(mr->lockers_in_use)++;
			pthread_cond_signal(& (mr->lock_empty_condition));
			pthread_mutex_unlock(&(mr->lock_available_mutex));
			break;
		}
	}

	int my_locker 	   = mr->claims[id];
	// lockers[my_locker] = kv;


	// serialize the kvpair...using another kvpair!
	struct kvpair * locker_contents = malloc(sizeof(struct kvpair));
	locker_contents->key   = &(*(kv->key));
	locker_contents->value = &(*(kv->value));
	locker_contents->keysz = kv->keysz;
	locker_contents->valuesz = kv->valuesz;

	// store a pointer to the new kvpair in my locker
	mr->lockers[my_locker] = locker_contents;

	// signal that there is something available to consume!

	return SUCCESS;
}

int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv) {

	while (mr->claims[id] == NO_CLAIM)
	{
		// wait on a CV for the locker contents itself
		pthread_cond_wait(&(mr->lock_empty_condition), &(mr->lock_available_mutex));
		return 0;
	}

	// lock the locker contents with a mutex
	pthread_mutex_lock(& (mr->lock_available_mutex) );

	int my_locker = mr->claims[id];

	// unserialize (or not really) the data
	kv = mr->lockers[my_locker];

	mr->locks[my_locker] = !(LOCKED);
	(mr->lockers_in_use)--;
	mr->claims[id] = NO_CLAIM;

	pthread_cond_signal(& (mr->lock_available_condition));
	pthread_mutex_unlock(& (mr->lock_available_mutex));
	
	return SUCCESS;
}
/*
	int my_locker = mr->claims[id];
	for(int i = 0; i<locker_count; i++ ){
		pthread_mutex_lock(&(mr->lock_available_mutex); // p1
		while ((mr->lockers_in_use == 0)
			pthread_cond_wait(&(mr->locker_empty_condition), &(mr->lock_available_mutex)); // p3
		
		mr->locks[my_locker] = !(LOCKED);
		(mr->lockers_in_use)--;
		mr->claims[id] = NO_CLAIM;
		kv = lockers[my_locker]

		pthread_cond_signal(& (mr->lock_available_condition));
		pthread_mutex_unlock(& (mr->lock_available_mutex));
		printf("%d\n", tmp);
	}
}
*/



/* test function */
/*void iofilefn(const char *inpath, const char *outpath) {
	char str1[5],str2[5],str3[5];
	FILE *fp = fopen(inpath,"r+");
	if (fp == NULL){
		printf("file not found!");
	}
	fscanf(fp, "%s %s %s",str1,str2,str3);

	FILE *fp1 = fopen(outpath,"w+");
	if (fp1 == NULL){
		printf("file not found!");
	}
	fprintf(fp1, "%s %s %s",str1,str2,str3);
}*/

/* for testing purposes */
//int main(){

	/* test 1 - no map/reduce functions yet */

/*	struct map_reduce * mr = mr_create(NULL, NULL, 8, 8);

	if (mr->map == NULL)
		printf("Map function is NULL!\n");

	if (mr->reduce == NULL)
		printf("Reduce function is NULL!\n");

	printf("threads: %d\n", mr->threads);
	printf("bufsize: %d\n", mr->buffer_size);

	mr_start(mr, NULL, NULL);

	mr_destroy(mr); 
*/
	/* test 2 - invalid numerical inputs */

/*	struct map_reduce * ms = mr_create(NULL, NULL, -1, 8);

	if (ms == NULL)
		printf("map_reduce is NULL!\n");

	mr_destroy(ms); 
*/
/*
	char *inpath = "/home/ugrads/tjb5698/473/Lab4/lab4/testfile.txt" ;
	char *outpath = "/home/ugrads/tjb5698/473/Lab4/lab4/output.txt" ;
	iofilefn(inpath, outpath);
*/	

//	return 0;
//}
