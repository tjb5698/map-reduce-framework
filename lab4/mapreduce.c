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

#define SUCCESS    1
#define FAILURE   -1

#define UNCLAIMED -1		// the locker is not claimed by any map thread
#define LOCKED    true		// the locker is locked
#define UNLOCKED  false		// the locker is not locked

bool debug = true;

struct args
{
	struct map_reduce * mr;
	int thread_id;
	int * reduce_count;
	int retval;
	int infd;
	int outfd;
};

int locker_count(struct map_reduce *mr);

/*
void sumarray_map( map_args_t * args_test ) {
    int nChunkSize  = args_test->length;
    // cout << "Map chunk size is " << nChunkSize << endl;
    int * miniArray = (int*) args_test->data;
  
    // cout << "Calculating intermediate sum over " << nChunkSize << " elements: " ;
  
    int intermediate_sum = 0;
    for(int i=0;i<nChunkSize;i++) {
      intermediate_sum += miniArray[i];
    }
  
    // cout << intermediate_sum << endl;
  
    int * key = (* int)1;
 //   *key = 1;
  
    int * val = (* int)intermediate_sum;
//    *val = intermediate_sum;
  
    // cout << "Emitting intermediate <" << *key << "," << *val << ">" << endl;
    store_temp( key, val, sizeof( int* ) );
  }

void sumarray_reduce( void * key_in, void ** vals_in, int vals_len ) {
    int nElements = vals_len;
    int ** p_array = (int**) vals_in;
    int * p_key = (int*) key_in;
  
    delete (p_key);
  
    int sum = 0;
    for(int i=0;i<nElements;i++) {
      sum += p_array[i][0];
      delete p_array[i];
    }
  
    int * key = new int;
    *key = 0;
 
    int * val = new int;
    *val = sum;
  
    store( key, val );
  }
*/

struct map_reduce *mr_create(map_fn map, reduce_fn reduce, int threads, int buffer_size)
{
	/* invalid map or reduce function */
	if (map == NULL || reduce == NULL)
		return NULL;

	/* invalid map thread count       */
	if (threads < 1)
		return NULL;

	/* invalid buffer size */
	if (buffer_size < 1)
		return NULL;

	/* create a new map_reduce struct */
  	struct map_reduce * mr = malloc(sizeof(struct map_reduce));

	if (mr == NULL)
		return NULL;

	/* set pointers to the map and reduce functions  */
	mr->map          = map;
	mr->reduce       = reduce;

	/* set number of map threads   */
	mr->map_count    = threads;
        mr->reduce_count = 1;
	
	/* set up thread pointer block */
	mr->mapThreads   = malloc((mr->map_count   ) * sizeof(pthread_t));
	mr->reduceThread = malloc((mr->reduce_count) * sizeof(pthread_t));

	/* set buffer size in bytes  */
	mr->buffer_size  = buffer_size;

	/* create the shared lockers (buffers) */
	mr->lockers      = malloc( locker_count(mr) * sizeof(struct kvpair) );
	
	/* set up the locks for the shared buffers */
	mr->locks 	 = malloc( locker_count(mr) * sizeof(bool));

	for (int i = 0; i < locker_count(mr); i++)
		// initially, every locker is unlocked
		(mr->locks)[i] = UNLOCKED;

	/* set up the claims list for the shared buffer */
	mr->claims       = malloc(mr->map_count    * sizeof(int));

	for (int j = 0; j < mr->map_count; j++)
		// no mapper thread has claimed a locker
		(mr->claims)[j] = UNCLAIMED;

	/* initially, there are no lockers in use */
	mr->lockers_in_use = 0;

	/* allocate space for and initialize the condition
	   variable and its corresponding mutex variable   */
	mr->lock_available_mutex      = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	mr->mrop_mutex                = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	mr->lock_available_condition  = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	mr->lock_empty_condition      = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	mr->mrop_complete_condition   = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

	mr->map_status = 0;

	/* return a pointer to the new map_reduce struct */
	return mr;
}

int locker_count(struct map_reduce *mr)
{
	if (mr == NULL)
		return -1;
	return mr->buffer_size / sizeof(struct kvpair);
}

void mr_destroy(struct map_reduce *mr)
{
	if (mr != NULL)
	{
		free(mr);
	}
}

void * mr_start_helper(void * myArgs)
{

	struct map_reduce * mr = ((struct args *) myArgs)->mr;
	int               infd = ((struct args *) myArgs)->infd;
	int          thread_id = ((struct args *) myArgs)->thread_id;

	// the actual function call to map()
	((struct args *)myArgs)->retval = mr->map(mr, infd, thread_id, mr->map_count);

	return NULL;
}

void * mr_reduce_helper(void * myArgs)
{
	printf("Entered reduce helper\n");	

	struct map_reduce * mr = ((struct args *) myArgs)->mr;
	int              outfd = ((struct args *) myArgs)->outfd;
        int     * reduce_count = ((struct args *) myArgs)->reduce_count;
	
//	printf("Entered reduce_helper\n");
	// the actual function call to reduce()
	((struct args *)myArgs)->retval = mr->reduce(mr, outfd, mr->map_count);

	printf("Returned from reduce fn\n");	

	if (((struct args *)myArgs)->retval == 0)
		*reduce_count = + 1;

	if (*reduce_count == mr->map_count)
		mr->map_status = 0;

	pthread_cond_signal(& (mr->mrop_complete_condition));	//signal that done 

	return NULL;
}

int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath)
{
	int * reduce_count = 0;

	int infd;

	struct args * myArgs = malloc(sizeof(struct args));

	for (int thread_id = 0; thread_id < mr->map_count; thread_id++)
	{
		if (debug)
			printf("Creating map thread %d\n", thread_id);

		// get a pointer to the thread
		pthread_t * mapThread = & ( (mr->mapThreads)[thread_id] );

		printf("Opening file descriptor\n");

		// create input file descriptor
		infd = open(inpath, O_RDONLY); 

		if (infd == FAILURE){
			printf("Input file open error on map thread %d.\n", thread_id);
			return FAILURE;
		}
		
		/* create the next map thread
			map(mr, infd, id, nmaps): map function */

		myArgs->mr   = mr;
		myArgs->infd = infd;
		myArgs->thread_id = thread_id;

		printf("Calling pthread_create for map thread\n");

		pthread_create(mapThread,		// the thread
				NULL,			// attributes
				mr_start_helper,
			  (void *) myArgs);

		// stop if thread creation not successful
		if (myArgs->retval != 0){
			printf("wtf\n");
			mr->map_status = -1;
			return FAILURE;
		}
	}

	printf("Opening output file descriptor\n");

	/* create output file descriptor */
	mr->outfd = open(outpath, O_WRONLY | O_CREAT, S_IRWXU);

	if (mr->outfd < SUCCESS)
	{
		printf("Output file open error on reduce thread.\n");
		return FAILURE;
	}

	printf("Calling pthread_create for reduce thread\n");

	/* create the reduce thread */

	myArgs->outfd = mr->outfd;
	myArgs->reduce_count = reduce_count;

	pthread_t * reduceThread = mr->reduceThread;

	pthread_create(reduceThread,
			NULL,
			mr_reduce_helper,
			(void *) myArgs);
	printf("OK\n");

	if (myArgs->retval != 0)
	{
		mr->map_status = -1;
		printf("wtf\n");
		return FAILURE;
	}
	return 0;
//	return SUCCESS;
}

int mr_finish(struct map_reduce *mr) 
{

	if((mr->map_status) == -1){
		printf("unsuccessful map-reduce op.\n");
		return FAILURE;
	}

	while ((mr->map_status) != 0)
		pthread_cond_wait( &(mr->mrop_complete_condition), & (mr->mrop_mutex) );

	printf("made it past the wait.\n");


	if (close(mr->outfd) < 0)
	{
		printf("Error closing output file descriptor.\n");
		return 1;
	}

	printf("Closing output file descriptor.\n");
	return 0;
}

int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv) {
	
	printf("%d - max kvpair size is %d\n",id,sizeof(struct kvpair));
	int size = kv->keysz + kv->valuesz;
	printf("%d - actual kv pair size is %d\n",id,size);	
	//mr->locker_count = 3;
	printf("%d - Entered mr produce.\n", id);
 
	// check if buffer is too small to hold even a single kvpair
	if ((mr->buffer_size) < sizeof(struct kvpair)){
		printf("%d - kv pair too big.\n", id);
		return FAILURE;
	}
	// mutex lock??
	printf("%d - Buffer size is big enough.\n",id);
	// if all the lockers are in use, wait until a locker is available
	while (mr->lockers_in_use == locker_count(mr))
	{
		printf("%d - All lockers are full.\n", id);
		pthread_cond_wait( &(mr->lock_available_condition), & (mr->lock_available_mutex) );
	}

	// a locker is available
	pthread_mutex_lock(& (mr->lock_available_mutex) );
	printf("%d - A locker is available!\n", id);

	// search for the empty locker
	for (int i = 0; i < (locker_count(mr)); i++)
	{
		if (mr->locks[i] != LOCKED)
		{
			// we found an unlocked locker!
			// "mine mine mine" (Finding Nemo reference)
			printf("%d - Locker %d is unlocked.\n",id,i);
			mr->claims[id] = i;
			mr->locks[i]   = LOCKED;
			printf("%d - Locker %d is locked.\n",id,i);
			(mr->lockers_in_use)++;
			printf("%d - Currently %d lockers are in use.\n",id,mr->lockers_in_use);
			pthread_cond_signal(& (mr->lock_empty_condition));
			pthread_mutex_unlock(&(mr->lock_available_mutex));
			break;
		}
	}

	int my_locker 	   = mr->claims[id];

	// serialize the kvpair...using another kvpair!
	struct kvpair * locker_contents = malloc(sizeof(struct kvpair));

	// allocate the key and the value using the keysz and valuesz in kv
	locker_contents->key   = malloc(kv->keysz);
	locker_contents->value = malloc(kv->valuesz);
 
	locker_contents->key   = kv->key;
	locker_contents->value = kv->value;

	// update the keysz and valuesz in the locker contents
	locker_contents->keysz   = kv->keysz;
	locker_contents->valuesz = kv->valuesz;

	// store the new locker contents in my locker
	mr->lockers[my_locker] = *locker_contents;
	
	//printf("Key is %d\n",(*((char*)(mr->lockers[my_locker]).key)));
	//printf("Value is %d\n",(*((int*)(mr->lockers[my_locker]).value)));	

	// signal that there is something available to consume!

	return SUCCESS;
}

int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv) {
	
	printf("Entered mr consume\n");

	while (mr->claims[id] == UNCLAIMED)
	{
		// wait on a CV for the locker contents itself
		pthread_cond_wait(&(mr->lock_empty_condition), &(mr->lock_available_mutex));
		return 0;
	}

	// lock the locker contents with a mutex
	pthread_mutex_lock(& (mr->lock_available_mutex) );

	int my_locker = mr->claims[id];

	// unserialize (or not really) the data
	kv = &(mr->lockers[my_locker]);

	mr->locks[my_locker] = !(LOCKED);
	(mr->lockers_in_use)--;
	mr->claims[id] = UNCLAIMED;

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
/*
int my_map (struct map_reduce *mr, int infd, int id, int nmaps){
	FILE *fpin = fdopen(infd, "r");	
	//create the kaey value pair
	struct kvpair *my_kvpair = malloc(sizeof(struct kvpair));
	//read till space character
	if ((char currChar = fgetc (fpin)) != ' ' &&  currChar != EOF)
		my_kvpair->key = currChar;
		my_kvpair->value = 1;
	}
	//call mr_produce
	int retval = mr_produce (*mr, id, *my_kvpair);
	//close file
	close(infd);
	if (retval != SUCCESS){
		printf("Error in mr produce.");
		return -1;
	}
	//return 0 if successful else failed
	return 0;
}

int my_reduce (struct map_reduce *mr, int outfd, int nmaps){
	FILE *fout = fdopen(outfd,"w");
}
*/

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
