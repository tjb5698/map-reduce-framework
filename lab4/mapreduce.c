/******************************************************************************
 * Your implementation of the MapReduce framework API.
 *
 * Other than this comment, you are free to modify this file as much as you
 * want, as long as you implement the program.
 *
 * Note: where the specification talks about the "caller", this is the program
 * which is not your code.  If the caller is required to do something, that
 * means your code may assume it has been done.
 ******************************************************************************/

#include "mapreduce.h"

/**
 * Allocates and initializes an instance of the MapReduce framework.  This
 * function should allocate a map_reduce structure and any memory or resources
 * that may be needed by later functions.
 *
 * map      Pointer to map callback function
 * reduce   Pointer to reduce callback function
 * threads  Number of worker threads to use
 *
 * Returns a pointer to the newly allocated map_reduce structure on success, or
 * NULL to indicate failure.
 */
struct map_reduce *mr_create(map_fn map, reduce_fn reduce, int threads) {

}

/**
 * Destroys and cleans up an existing instance of the MapReduce framework.  Any
 * resources which were acquired or created in mr_create should be released or
 * destroyed here.
 *
 * mr  Pointer to the instance to destroy and clean up
 */
void mr_destroy(struct map_reduce *mr) {

}

/**
 * Begins a multithreaded MapReduce operation.  This operation will process data
 * from the given input file and write the result to the given output file.
 *
 * mr       Pointer to the instance to start
 * inpath   Path to the file from which input is read.  The framework should
 *          make sure that each Map thread gets an independent file descriptor
 *          for this file.
 * outpath  Path to the file to which output is written.
 */
int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath) {

}

/**
 * Blocks until the entire MapReduce operation is complete.  The caller is
 * required to close the input file descriptor before calling this function.
 *
 * mr  Pointer to the instance to wait for
 */
int mr_finish(struct map_reduce *mr) {

}

/**
 * Called by a Map thread each time it produces a key-value pair to be consumed
 * by the Reduce thread.  If the framework cannot currently store another
 * key-value pair, this function should block until it can.
 *
 * mr  Pointer to the MapReduce instance
 * id  Identifier of this Map thread, from 0 to (nmaps - 1)
 * kv  Pointer to the key-value pair that was produced by Map.  This pointer
 *     belongs to the caller, so you must copy the key and value data if you
 *     wish to store them somewhere.
 *
 * Returns 1 if one key-value pair is successfully produced (success), -1 on
 * failure.
 */
int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv) {

}

/**
 * Called by the Reduce function to consume a key-value pair from a given Map
 * thread.  If there is no key-value pair available, this function should block
 * until one is produced (in which case it will return 1) or the specified Map
 * thread returns (in which case it will return 0).
 *
 * mr  Pointer to the MapReduce instance
 * id  Identifier of Map thread from which to consume
 * kv  Pointer to the key-value pair that was produced by Map.  The caller is
 *     responsible for allocating memory for the key and value ahead of time and
 *     setting the pointer and size fields for each to the location and size of
 *     the allocated buffer.
 *
 * Returns 1 if one pair is successfully consumed, 0 if the Map thread returns
 * without producing any more pairs, or -1 on error.
 */
int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv) {

}
