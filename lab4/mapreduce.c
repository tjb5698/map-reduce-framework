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
	
	/* return a pointer to the new map_reduce struct */
	return mr;
}

void mr_destroy(struct map_reduce *mr) {
	if (mr != NULL)
		free(mr);
}

int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath) {
	return -1;
}

int mr_finish(struct map_reduce *mr) {
	return -1;
}

int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv) {
	return -1;
}

int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv) {
	return -1;
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

	mr_destroy(mr);

	/* test 2 - invalid numerical inputs */

	struct map_reduce * ms = mr_create(NULL, NULL, -1, 8);

	if (ms == NULL)
		printf("map_reduce is NULL!\n");

	mr_destroy(ms);

	return 0;
}
