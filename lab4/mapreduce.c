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

}

void mr_destroy(struct map_reduce *mr) {

}

int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath) {

}

int mr_finish(struct map_reduce *mr) {

}

int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv) {

}

int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv) {

}
