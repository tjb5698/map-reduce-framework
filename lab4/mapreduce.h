#ifndef MAPREDUCE_H_
#define MAPREDUCE_H_

/******************************************************************************
 * Definition of the MapReduce framework API.
 *
 * The ONLY changes you may make to this file are to add your data members to
 * the map_reduce struct definition.  Additionally, you are allowed to add
 * #includes for any types you need. You may also add your own struct
 * definitions.  Making any other changes to the function declarations below
 * alters the API, which breaks compatibility with all of the other programs
 * that are using your framework!
 *
 * Note: where the specification talks about the "caller", this is the program
 * which is not your code.  If the caller is required to do something, that
 * means your code may assume it has been done.
 ******************************************************************************/

/* Header includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Forward-declaration, the definition to edit is farther down */
struct map_reduce;

/*
 * Type aliases for callback function pointers.  These are functions you will be
 * passed by the caller.  All of them will return 0 to indicate success and
 * nonzero to indicate failure.
 */

/**
 * Function signature for caller-provided Map functions.  A Map function will
 * read input using the file descriptor infd, process it, and call mr_produce
 * for each key-value pair it outputs.  The framework must give each Map thread
 * an independent input file descriptor so they do not interfere with each
 * other.
 *
 * Since there will be many Map threads, each one should be given a unique id
 * from 0 to (nmaps - 1).
 */
typedef int (*map_fn)(struct map_reduce *mr, int infd, int id, int nmaps);

/**
 * Function signature for caller-provided Reduce functions.  A Reduce function
 * will receive key-value pairs from the Map threads by calling mr_consume,
 * combine them, and write the result to outfd.  The nmaps parameter, as above,
 * informs the Reduce function how many Map threads there are.
 */
typedef int (*reduce_fn)(struct map_reduce *mr, int outfd, int nmaps);

/* End struct section */

/*
 * Structure for storing any needed persistent data - do not use global
 * variables when writing a system!  You may put whatever data is needed by your
 * functions into this struct definition.
 *
 * This type is treated as "opaque" by the caller, which means the caller must
 * not manipulate it in any way other than passing its pointer back to your
 * functions.
 */
struct map_reduce {

    /* pointer to  map   function */
    map_fn          map;

    /* pointer to reduce function */
    reduce_fn       reduce;

    /* number of map threads */
    int             map_count;

    /* buffer size in bytes */
    int             buffer_size;

    /* array of map threads */
    pthread_t     * mapThreads;
    pthread_t     * reduceThread;

    /* shared buffer */
    int             locker_count;
    int             lockers_in_use;
    int           * claims;
    struct kvpair * lockers;
    bool          * locks;

    /* file descriptors */
    int * infd;
    int * outfd;

    /* synchronization primitives */
    pthread_mutex_t mrop_mutex;
    pthread_cond_t  mrop_complete_condition;

    /* synchronization primitives */
    pthread_mutex_t lock_available_mutex;
    pthread_cond_t  lock_available_condition;
    pthread_cond_t  lock_empty_condition;

    /* */
    int             map_status;
    
    // int buffer_space 	: space left in buffer
    // buffer Buff 			: buffer to serve as a memory between mapper and reducer 
    // sem_t empty			: signal from consumer to producer if buffer is empty
    // sem_t full			: signal from producer to consumer if buffer is full
    // sem_t bs_mutex		: lock for buffer_space
};

/**
 * Structure which represents an arbitrary key-value pair.  This structure will
 * be used for communicating between Map and Reduce threads.  In this framework,
 * you do not need to parse the information in the key or value, only pass it on
 * to the next stage.
 */
struct kvpair {
    /* Pointers to the key and value data */
    void *key;
    void *value;

    /* Size of the key and value data in bytes */
    uint32_t keysz;
    uint32_t valuesz;
};

/*
 * MapReduce function API
 *
 * These are the six functions you will be implementing in mapreduce.c.
 */

/**
 * Allocates and initializes an instance of the MapReduce framework.  This
 * function should allocate a map_reduce structure and any memory or resources
 * that may be needed by later functions.
 *
 * map          Pointer to map callback function
 * reduce       Pointer to reduce callback function
 * threads      Number of mapper threads to use
 * buffer_size  Size of the buffer between each mapper and the reducer
 *              (in bytes)
 *
 * Returns a pointer to the newly allocated map_reduce structure on success, or
 * NULL to indicate failure.
 */
struct map_reduce *mr_create(map_fn map, reduce_fn reduce, int threads, int buffer_size);

/**
 * Destroys and cleans up an existing instance of the MapReduce framework.  Any
 * resources which were acquired or created in mr_create should be released or
 * destroyed here.
 *
 * mr  Pointer to the instance to destroy and clean up
 */
void mr_destroy(struct map_reduce *mr);

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
int mr_start(struct map_reduce *mr, const char *inpath, const char *outpath);

/**
 * Blocks until the entire MapReduce operation is complete.  The caller is
 * required to close the input file descriptor before calling this function.
 *
 * mr  Pointer to the instance to wait for
 */
int mr_finish(struct map_reduce *mr);

/**
 * Called by a Map thread each time it produces a key-value pair to be consumed
 * by the Reduce thread.  If the framework cannot currently store another
 * key-value pair, this function should block until it can. If the entry is
 * larger than the entire buffer, then this function should fail (return -1).
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
int mr_produce(struct map_reduce *mr, int id, const struct kvpair *kv);

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
 *     the allocated buffer. After, the data has been read into the pair, this
 *     function should update the size fields to reflect the actual size of the
 *     data.
 *
 * Returns 1 if one pair is successfully consumed, 0 if the Map thread returns
 * without producing any more pairs, or -1 on error.
 */
int mr_consume(struct map_reduce *mr, int id, struct kvpair *kv);

#endif
