#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include "list.h"
#include "threadpool.h"

//~ DATA STRUCTURE DEFINITIONS ================================== ~//
//
/*
 * Thread Pool
 */
struct thread_pool {
	struct list * work_queue;       // Pending Tasks
	struct list * worker_threads;   // Thread roster
	pthread_mutex_t mutex;          // Synchronization lock
	pthread_cond_t condition;       // Broadcast condition
	bool is_shutting_down;          // Shutdown status

	size_t running_threads;
	size_t total_threads;
};
//
/*
 * Single-Threaded Task
 */
struct future {
	thread_pool_callable_func_t callable;   // function pointer
	void * callable_data;                   // parameters
	void * callable_result;                 // calculated result
	sem_t semaphore;                        // completion status
};
//
/*
 * DUCT-TAPE - Future
 */
struct future_dt {
	struct future * task;    // payload
	struct list_elem elem;   // list link
};
//
/*
 * DUCT-TAPE - Thread
 */
struct thread_dt {
	pthread_t thread;        // payload
	struct list_elem elem;   // list link
};



//~ HELPER FUNCTION PROTOTYPE(S) ================================ ~//
//
static void * worker_thread_body(void *);



//~ INTERFACE IMPLEMENTATION ==================================== ~//
//
/**
 * Create a new thread pool with n threads. 
 */
struct thread_pool * thread_pool_new(int nthreads) {

	//- Initialize thread pool object --------------------=
	//
	// Allocate memory
	struct  thread_pool * pool = (struct thread_pool*) malloc(sizeof(struct thread_pool));
	//
	// Set up work queue
	pool->work_queue = (struct list*) malloc(sizeof(struct list));
	list_init(pool->work_queue);
	//
	// Set up worker threads
	pool->worker_threads = (struct list*) malloc(sizeof(struct list));
	list_init(pool->worker_threads);
	//
	// Initialize synchronization fields
	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->condition, NULL); 
	pool->is_shutting_down = false;

	//- Load threads into pool ---------------------------=
	//
	int i = 0;
	for (; i < nthreads; i++) {
		// Build DT object
		struct thread_dt * tape = malloc(sizeof(struct thread_dt));
		pthread_create(&tape->thread, NULL, worker_thread_body, (void *) pool);
		//
		// Push to thread roster
		list_push_back(pool->worker_threads, &tape->elem);
		pool->total_threads++;
	}

	return pool;
}
//
/**
 * Shutdown this thread pool.
 *
 * Does not execute pending (not started) tasks.
 */
void thread_pool_shutdown(struct thread_pool * pool) {
	//- Enter shut-down mode -----------------------------=
	//
	// Atomic Write: shut down
	pthread_mutex_lock(&pool->mutex);                           //
	pool->is_shutting_down = true;                              //
	pthread_mutex_unlock(&pool->mutex);                         //

	//- Terminate threads + remove from roster -----------=
	//
	// Wake up threads from conditional wait
	pthread_mutex_lock(&pool->mutex);                           //
	pthread_cond_broadcast(&pool->condition);                   //
	pthread_mutex_unlock(&pool->mutex);                         //
	//
	while (! list_empty(pool->worker_threads)) {
		// remove from roster
		struct list_elem * e = list_pop_front(pool->worker_threads);
		struct thread_dt * tape = list_entry(e, struct thread_dt, elem);
		pool->total_threads--;
		//
		// reap thread
		pthread_join(tape->thread, NULL);
		//
		// deallocate DT
		free(tape);
	}

	//- Remove unstarted tasks from work queue -----------=
	//
	while(! list_empty(pool->work_queue)) {
		// Remove from work queue
		// (No atomic access req'd, all workers are dead.)
		struct list_elem * e = list_pop_front(pool->work_queue);
		struct future_dt * tape = list_entry(e, struct future_dt, elem);
		//
		// Deallocate
		future_free(tape->task);
		free(tape);
	}

	//- Tear down pool object ----------------------------=
	// 
	// Disable syncrhonization fields
	pthread_cond_destroy(&pool->condition);
	pthread_mutex_destroy(&pool->mutex);
	//
	// Deallocate
	free(pool->worker_threads);
	free(pool->work_queue);
	free(pool);
}
//
/**
 * Submit a callable to thread pool and return future.
 * The returned future can be used in future_get() and future_free()
 */
struct future * thread_pool_submit( struct thread_pool * pool,
	thread_pool_callable_func_t callable, 
	void * callable_data     ) {
	
	//- Initialize Future --------------------------------=
	//
	// Allocate memory
	struct future * task = (struct future *) malloc(sizeof(struct future));
	// Initialize Fields
	task->callable = callable;
	task->callable_data = callable_data;
	task->callable_result = NULL;
	// Initialize Semaphore
	sem_init(&task->semaphore, 0, 0);

	//- Add Future to work queue -------------------------=
	//
	// Build DT object
	struct future_dt * tape = malloc(sizeof(struct future_dt));
	tape->task = task;
	//
	// Add to work queue
	pthread_mutex_lock(&pool->mutex);                           //
	list_push_back(pool->work_queue, &tape->elem);              //
	pthread_mutex_unlock(&pool->mutex);                         //

	//- Notify threads of new job ------------------------=
	//
	// Wake up all threads in conditional wait
	pthread_mutex_lock(&pool->mutex);                           //
	pthread_cond_broadcast(&pool->condition);                   //
	pthread_mutex_unlock(&pool->mutex);                         //

	return task;
}
//
/**
 * Make sure that thread pool has completed executing this callable,
 * then return result. 
 */
void * future_get(struct future * task) {
	sem_wait(&task->semaphore);
	return task->callable_result;
}
//
/**
 * Deallocate this future.
 *
 * Must be called after future_get() 
 */
void future_free(struct future * task) {
	sem_destroy(&task->semaphore);
	free(task);
}


//~ HELPER FUNCTION(S) IMPLEMENTATION =========================== ~//
//
/**
 * Primary code block for a running worker thread.
 *
 * The thread enters this function, and is immediately put into
 * 	a conditional wait. It should only execute jobs if one has been
 *  formally submitted to the pool.
 *
 * As long as there is a job in the work queue, a running thread with
 *  no current task will claim that job and execute it. Unless, of
 *  course, the thread pool is entering shutdown mode.
 * 
 * Essentially, all of this function's execution will be insulated
 *  with a mutex lock. the only portion of this code that is in an
 *  unlocked state (executing in parallel), is when this thread 
 *  begins execution of its retrieved task, which should be running
 *  in parallel anyways. This broad lock is required to make sure
 *  that no peer thread can alter work queue data or synchronization
 *  fields in a way that would generate a race condition.
 */
static void * worker_thread_body(void * param) {
	struct thread_pool * pool = (struct thread_pool *) param;
	
	// Atomic Access: shut down
	pthread_mutex_lock(&pool->mutex);                           // 
	bool cont = ! pool->is_shutting_down;                       //
	pthread_mutex_unlock(&pool->mutex);                         //
	
	while (cont) {
		//- Assume wait state until dispatched -----------=
		//
		// Wait for broadcast on the condition
		pthread_mutex_lock(&pool->mutex);                       //
		pthread_cond_wait(&pool->condition, &pool->mutex);      //
		pthread_mutex_unlock(&pool->mutex);                     //

		pthread_mutex_lock(&pool->mutex);                       //
		bool c = ! list_empty(pool->work_queue);                //
                                                                //
		while (cont && c) {                                     //
			struct future * task = NULL;                        //
			struct list_elem * e = NULL;                        //
			struct future_dt * tape = NULL;                     //
                                                                //
			//- Retrieve task ----------------------------=     //
			//                                                  //
			// Atomic Access: Work Queue                        //
			e = list_pop_front(pool->work_queue);               //
			tape = list_entry(e, struct future_dt, elem);       //
			task = tape->task;                                  //
			pool->running_threads++;                            //

			//- Execute task -----------------------------=
			//
			pthread_mutex_unlock(&pool->mutex);
			void * result = task->callable(task->callable_data);
			pthread_mutex_lock(&pool->mutex);

			//- Report results of task -------------------=     //
			//                                                  //
			task->callable_result = result;                     //
			sem_post(&task->semaphore);                         //
			pool->running_threads--;                            //
                                                                //
			c = ! list_empty(pool->work_queue);                 //
		}                                                       //
                                                                //
		cont = ! pool->is_shutting_down;                        //
		pthread_mutex_unlock(&pool->mutex);                     //
	}

	return NULL;
}

int thread_pool_num_running_threads(struct thread_pool * pool) {
	return pool->running_threads;
}

int thread_pool_num_total_threads(struct thread_pool * pool) {
	return pool->total_threads;
}