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
	mr->map         = map;
	mr->reduce      = reduce;
	mr->threads     = threads;
	mr->buffer_size = buffer_size;

	/* set up thread block */
	mr->theThreads        = malloc(threads * sizeof(pthread_t));
	
	//mr->buffer_space = buffer_size; Keeps track of space left in buffer
	
	/* return a pointer to the new map_reduce struct */
	return mr;
}

void mr_destroy(struct map_reduce *mr) {
	if (mr != NULL)
	{
		free(mr);
	}
}

int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath) {
	for (int thread_id = 0; thread_id < mr->threads; thread_id++)
	{
		// get a pointer to the thread
		pthread_t * theThread = & ( (mr->theThreads)[thread_id] );

		// create the next mapper thread
		/* map(mr, infd, id, nmaps): mapper function */
		int retval = pthread_create(theThread,		/* the thread */
						NULL,
						helloWorld,
				       (void *) thread_id);
		//				NULL,		/* attributes */
		//				mr->map, 	/*  function  */
		//	  	       (void *) mr,		/*    arg0    */
		//	  	       (void *)	infd, 		/*    arg1    */
		//	 	       (void *)	outfd, 		/*    arg2    */
		//	  	       (void *)	&thread_id);	/* thread  ID */


		// stop if thread creation not successful
		if (retval != 0)
			return 1;
	}

	/* thread creation successful */
	return 0;
}

void *helloWorld( void * thread_id )
{
	long tid = (long) thread_id;
	printf("Hello from thread %ld!\n", tid);
	pthread_exit(NULL);
}

int mr_finish(struct map_reduce *mr) {
	return -1;
}

int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv) {
//	kvpair produced by map fn
//	int kvsize = kv->keysz + kv->valuesz
//	if (kvsize > mr->buffer_size)
//		return -1;
//	else 
//		while (kv not stored: kvsize > buffer_space) 
//			block mapper thread;		
//		add kv pair to buffer
//		mutex lock buffer_space
//		update mr->buffer_space = mr->buffer_space - kvsize 
//		mutex unlock buffer_space
//		signal consumer
//		return 1;	
//			


	/* producer fn using semaphores */
/*	int i;
	for (i = 0; i < loops; i++) {
		sem_wait(&empty); // Line P1
		sem_wait(&mutex); // Line P1.5 (MOVED MUTEX HERE...)
		put(i); // Line P2
		sem_post(&mutex); // Line P2.5 (... AND HERE)
		sem_post(&full); // Line P3
 	} */

	return -1;
}

int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv) {
	return -1;
}

void iofilefn(const char *inpath, const char *outpath) {
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
}

int main(){

	/* test 1 - no map/reduce functions yet */

	struct map_reduce * mr = mr_create(NULL, NULL, 8, 8);

	if (mr->map == NULL)
		printf("Map function is NULL!\n");

	if (mr->reduce == NULL)
		printf("Reduce function is NULL!\n");

	printf("threads: %d\n", mr->threads);
	printf("bufsize: %d\n", mr->buffer_size);

	mr_start(mr, NULL, NULL);

	mr_destroy(mr); 

	/* test 2 - invalid numerical inputs */

	struct map_reduce * ms = mr_create(NULL, NULL, -1, 8);

	if (ms == NULL)
		printf("map_reduce is NULL!\n");

	mr_destroy(ms); 

/*
	char *inpath = "/home/ugrads/tjb5698/473/Lab4/lab4/testfile.txt" ;
	char *outpath = "/home/ugrads/tjb5698/473/Lab4/lab4/output.txt" ;
	iofilefn(inpath, outpath);
*/	

	return 0;
}
