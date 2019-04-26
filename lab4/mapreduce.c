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

#define UNCLAIMED -1		// the locker is not claimed by any map thread
#define LOCKED    true		// the locker is locked
#define UNLOCKED  false		// the locker is not locked

struct args
{
	struct map_reduce * mr;
	int thread_id;
	int map_create_retval;
	int reduce_create_retval;
	int infd;
	int outfd;
};

int locker_count(struct map_reduce *mr);

struct map_reduce *mr_create(map_fn map, reduce_fn reduce, int threads, int buffer_size)
{
	/* invalid map or reduce function */
	if (map == NULL || reduce == NULL)
		return NULL;

	/* invalid map thread count       */
	if (threads < 1)
		return NULL;

	/* invalid buffer size            */
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
	
	/* no maps are done yet */
	mr->nmaps_done   = 0;

	/* mutex for nmaps_done */
	mr->nmaps_done_mutex
			         = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

	/* set up thread pointer block */
	mr->mapThreads   = malloc((mr->map_count   ) * sizeof(pthread_t));
	mr->reduceThread = malloc((mr->reduce_count) * sizeof(pthread_t));

	/* set buffer size in bytes  */
	mr->buffer_size  = buffer_size;

	/* create the shared lockers (buffers) */
	mr->lockers      = malloc(locker_count(mr) * sizeof(struct kvpair));
	
	/* set up the locks for the shared buffers */
	mr->locks 	     = malloc(locker_count(mr) * sizeof(bool));

	for (int i = 0; i < locker_count(mr); i++)
	{
		/* initially, every locker is unlocked */
		(mr->locks)[i] = UNLOCKED;
	}

	/* set up the claims list for the shared buffer */
	mr->claims       = malloc(mr->map_count    * sizeof(int));

	for (int j = 0; j < mr->map_count; j++)
	{
		/* initially, every locker is unclaimed */
		(mr->claims)[j] = UNCLAIMED;
	}

	/* initially, there are no lockers in use */
	mr->lockers_in_use = 0;

	/* mutex for locks array */
	mr->locks_mutex			  = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

	/* condition variables and their mutex buddies */

	/* is the lock  full? */
	mr->locker_full_mutex  = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	mr->locker_full_cv     = (pthread_cond_t)  PTHREAD_COND_INITIALIZER;

	/* is the locker empty? */
	mr->locker_empty_mutex  = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	mr->locker_empty_cv     = (pthread_cond_t)  PTHREAD_COND_INITIALIZER;

    /* is the map thread done? */
	mr->map_complete_mutex    = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	mr->map_complete_cv       = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

	/* is the mapreduce operation done? */
	mr->mapreduce_complete_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
	mr->mapreduce_complete_cv    = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

	/* mapreduce status code */
	mr->status_code = 0;

	printf("Created mr framework\n");

	/* return a pointer to the new map_reduce struct */
	return mr;
}

int locker_count(struct map_reduce *mr)
{
	if (mr == NULL)
		return -1;
	return mr->buffer_size/sizeof(struct kvpair);
}

void mr_destroy(struct map_reduce *mr)
{
	if (mr != NULL)
	{
		free(mr);
	}
}

void *mr_map_helper(void *myArgs)
{
	/* set up the arguments for the map function */
	struct map_reduce *mr  = ((struct args *) myArgs)->mr;
	int               infd = ((struct args *) myArgs)->infd;
	int          thread_id = ((struct args *) myArgs)->thread_id;

	/* call the map function */
	((struct args *) myArgs)->map_create_retval = (mr->map)(mr, infd, thread_id, mr->map_count);

	/* update the number of maps done */
	if (((struct args *)myArgs)->map_create_retval == 0)
	{
		pthread_mutex_lock(&(mr->nmaps_done_mutex));
		(mr->nmaps_done)++;
		pthread_mutex_unlock(&(mr->nmaps_done_mutex));
		
		printf("%d - map threads done = %d\n",thread_id, mr->nmaps_done);
		printf("%d - total map threads = %d\n",thread_id, mr->map_count);
		/* the last map thread should turn off the lights and close and lock the door */
		if (mr->nmaps_done >= 1)
		{
			pthread_cond_signal(&(mr->map_complete_cv));
		}
	}

	/* the null pointer! */
	return NULL;
}

void *mr_reduce_helper(void *myArgs)
{
	/* set up the arguments for the reduce function */
	struct map_reduce *mr  = ((struct args *) myArgs)->mr;
	int              outfd = ((struct args *) myArgs)->outfd;

	printf("Another statement\n");
	//printf("Total number of map threads = %d \n",mr->map_count);

	//printf("Number of map threads done = %d \n",mr->nmaps_done);

	/* loop until all of the maps are done */
//	while (mr->nmaps_done <= mr->map_count)
//	{
	while(mr->nmaps_done < (mr->map_count-1))
	{
		printf("Reduce waiting for map thread to complete\n");
		/* wait for a map thread to complete */
		
		pthread_cond_wait(&(mr->map_complete_cv), &(mr->map_complete_mutex));
		printf("Done waiting\n");
		/* call the reduce function */
		((struct args *) myArgs)->reduce_create_retval = (mr->reduce)(mr, outfd, mr->map_count);
	}
//	((struct args *) myArgs)->reduce_create_retval = (mr->reduce)(mr, outfd, mr->map_count);
	
//	if (mr->nmaps_done != mr->map_count)
//	{
//		pthread_cond_wait(&(map_complete_cv), &(map_complete_mutex));
//	}

	/* the null pointer! */
	return NULL;
}

int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath)
{
	printf("Entered mr_start\n");

	/* try to initialize the output file descriptor */
	mr->outfd = open(outpath, O_WRONLY | O_CREAT, S_IRWXU);

	/* check for errors */
	if (mr->outfd < 0)
	{
		printf("I couldn't open the output file descriptor for the reduce thread.\n");
		
		mr->status_code = 1;

		/* signal that the mapreduce workflow is done */
		pthread_cond_signal(&(mr->mapreduce_complete_cv));

		return 1;
	}

	/* get a pointer to the reduce thread */
	pthread_t *reduceThread  = mr->reduceThread;

	/* set up the reduce_args struct */
	struct args *reduce_args = malloc(sizeof(struct args));
	
	reduce_args->mr			  =mr;
	reduce_args->outfd 		  = mr->outfd;
	reduce_args->reduce_create_retval = 0;

	/* create the reduce thread */
	pthread_create(reduceThread, NULL, mr_reduce_helper, (void *) reduce_args);

	/* check for errors */
	if (reduce_args->reduce_create_retval != 0)
	{
		printf("I couldn't create the reduce thread.\n");

		mr->status_code = 1;

		/* signal that the mapreduce workflow is done */
		pthread_cond_signal(&(mr->mapreduce_complete_cv));

		return 1;
	}
	if (mr->nmaps_done == mr->map_count)
	{
		pthread_cond_signal(&(mr->mapreduce_complete_cv));
		return 0;		
	}
	/* declare the input file descriptor */
	int infd;

	/* create the map threads */
	for (int thread_id = 0; thread_id < mr->map_count; thread_id++)
	{
		printf("%d - creating file descriptor\n",thread_id);
		/* get a pointer to the next map thread */
		pthread_t *mapThread = &((mr->mapThreads)[thread_id]);

		/* try to initialize the input file descriptor */
		infd = open(inpath, O_RDONLY); 

		if (infd < 0)
		{
			printf("I couldn't open the input file descriptor for the map thread with id =%d.\n", thread_id);

			mr->status_code = 1;

			/* signal that the mapreduce workflow is done */
			pthread_cond_signal(&(mr->mapreduce_complete_cv));

			return 1;
		}
		
		printf("%d - input file opened successfully\n",thread_id);
		/* set up the map_args struct */
		struct args *map_args   = malloc(sizeof(struct args));

		map_args->mr        = mr;
		map_args->infd      = infd;
		map_args->thread_id = thread_id;
		map_args->map_create_retval = 0;

		/* create the next map thread */
		pthread_create(mapThread, NULL, mr_map_helper, (void *) map_args);

		/* check for errors */
		if (map_args->map_create_retval != 0)
		{
			printf("I couldn't create the map thread with id=%d.\n", thread_id);

			mr->status_code = 1;

			/* signal that the mapreduce workflow is done */
			pthread_cond_signal(&(mr->mapreduce_complete_cv));

			return 1;
		}
	}

	// done
	return 0;
}

int mr_finish(struct map_reduce *mr) 
{
	/* the cookie monster ate all the cookies */
	if(mr->status_code == 0)
	{
		/* wait until the entire mapreduce workflow is complete */
		pthread_cond_wait(&(mr->mapreduce_complete_cv), &(mr->mapreduce_complete_mutex));

		/* try to close the output file descriptor */
		if (close(mr->outfd) < 0)
		{
			printf("I couldn't close the output file descriptor.\n");
			mr->status_code = 1;
		}
	}

	return mr->status_code;
}

int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv)
{
	printf("%d - Entered mr_produce\n",id);

	/* check if buffer is too small to hold even a single kvpair */
	if ((mr->buffer_size) < sizeof(struct kvpair))
	{
		printf("Square pegs don't go in round holes.\n");
		
		mr->status_code = 1;

		/* signal that the mapreduce workflow is done */
		pthread_cond_signal(&(mr->mapreduce_complete_cv));

		return -1;
	}
	
	printf("%d - kvpair fits in buffer\n",id);	
	//printf("%d - %d lockers in use right now\n",id, mr->lockers_in_use);
	//printf("%d - there are total %d lockers\n", id, locker_count(mr));
	/* if all the lockers are in use, wait until a locker is available */
	while (mr->lockers_in_use == locker_count(mr))
	{
		printf("%d - Waiting for a locker to be emptied\n",id);
		pthread_cond_wait(&(mr->locker_full_cv), &(mr->locker_full_mutex));
	}

	printf("%d - locker is available\n",id);
	/* a locker is available! */
	pthread_mutex_lock(&(mr->locks_mutex));

	/* search for the empty locker */
	for (int i = 0; i < locker_count(mr); i++)
	{
		if (mr->locks[i] != LOCKED)
		{
			printf("%d - claimed locker %d and locks it.\n",id,i); 
			/* we found an unlocked locker! */
			mr->claims[id] = i;
			mr->locks [i]  = LOCKED;
			(mr->lockers_in_use)++;
			printf("%d lockers are being used now\n",mr->lockers_in_use);

			pthread_mutex_unlock(&(mr->locks_mutex));
			break;
		}
	}

	/* what locker do I have? */
	int my_locker 	   = mr->claims[id];

	/* serialize the kvpair...using another kvpair! */
	struct kvpair *locker_contents = malloc(sizeof(struct kvpair));

	/* allocate the key and the value using the keysz and valuesz in kv */
	locker_contents->key   = malloc(kv->keysz);
	locker_contents->value = malloc(kv->valuesz);
 
 	/* copy the key and the value to the new kvpair */
	locker_contents->key   = kv->key;
	locker_contents->value = kv->value;

	/* update keysz and valuesz in the new kvpair */
	locker_contents->keysz   = kv->keysz;
	locker_contents->valuesz = kv->valuesz;

	/* store the new locker contents in my locker */
	mr->lockers[my_locker] = *locker_contents;

	printf("%d\tProducing %p\n: %s - %d\n",id,locker_contents, (char*)(kv->key),*((int*)(kv->value)));
	
	/* signal that a locker has data */
	pthread_cond_signal(&(mr->locker_empty_cv));

	// done
	return 1;
}

int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv)
{
	printf("%d - Entered mr consume\n",id);
	/* wait until the locker has data */
	if(mr->nmaps_done == mr->map_count)
	{
		return 1;	
	}
	
	if(mr->claims[id] == UNCLAIMED)
	{
		return 0;
	}
	while(mr->lockers_in_use == 0)
	{
		printf("id = %d: Waiting for locker to be filled\n",id);
		pthread_cond_wait(&(mr->locker_empty_cv), &(mr->locker_empty_mutex));
	}
	
	/* what locker do I have? */
	int my_locker = mr->claims[id];
	printf("%d - I have a locker %d\n",id,my_locker);

	/* unserialize (or not really) the data */
	kv = &(mr->lockers[my_locker]);
	printf("%d\tConsuming %p\n: %s - %d\n",id,kv,(char*)(kv->key),*((int*)(kv->value)));

	/* mark the locker as unlocked and unclaimed*/
	pthread_mutex_lock(&(mr->locks_mutex));
	mr->locks[my_locker] = !(LOCKED);
	printf("%d - Locker %d is now unlocked\n",id,my_locker);
	(mr->lockers_in_use)--;
	printf("%d - %d lockers are in use\n",id,mr->lockers_in_use);
	mr->claims[id] = UNCLAIMED;
	printf("%d - I dont have a locker and now my claim is %d\n",id,mr->claims[id]);
	pthread_mutex_unlock(&(mr->locks_mutex));

	/* signal that a locker is now empty */
	pthread_cond_signal(&(mr->locker_full_cv));
	
	// done
	return 1;
}
