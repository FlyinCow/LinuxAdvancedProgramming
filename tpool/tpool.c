#include <pthread.h>
#include <stdlib.h>

typedef enum OP { ADD, SUB, MUT, DIV } OP;

int calc(int a, int b, OP op) {
  switch (op) {
  case ADD:
    return a + b;
  case SUB:
    return a - b;
  case MUT:
    return a * b;
  case DIV:
    return a / b;
  }
}

typedef struct job_t {
  struct job_t *prev;
  struct job_t *next;
  OP op;
  int a;
  int b;
} job_t;

typedef struct job_queue_t {
  job_t *head;
  job_t *tail;
  int job_count;
} job_queue_t;

// every thread has a job queue, a mutex and a cond
typedef struct ithread_t {
  pthread_t thread;
  pthread_mutex_t mutex; // for job queue
  pthread_cond_t cond;   // for job queue
  job_queue_t job_queue;
} ithread_t;

typedef struct tpool_t {
  int thread_count;
  ithread_t *due;
  pthread_mutex_t _m;
  ithread_t *threads;
} tpool_t;

static tpool_t pool;

/**
 * @brief init thread pool at `pool` with `num_threads` threads
 *
 * @param pool
 * @param num_threads
 * @return int
 */
int tpool_create(tpool_t *pool, int num_threads) {
  if (num_threads <= 0)
    num_threads = 1; // at least one thread in the pool
  pool->thread_count = num_threads;

  if (0 != pthread_mutex_init(&pool->_m, NULL))
    goto return_error;

  // init ithread_t
  if (NULL == (pool->threads = calloc(num_threads, sizeof(ithread_t))))
    goto return_error;
  for (int i = 0; i < num_threads; i++) {
    if (0 != pthread_mutex_init(&pool->threads[i].mutex, NULL))
      goto cleanup_ithreads;
    if (0 != pthread_cond_init(&pool->threads[i].cond, NULL))
      goto cleanup_ithreads;
    pool->threads[i].job_queue.head = pool->threads->job_queue.tail = NULL;
    pool->threads->job_queue.job_count = 0;
  }
  // todo: create threads
  return 0;

cleanup_ithreads:
  free(pool->threads);
return_error:
  return -1;
}

void tpool_destroy(tpool_t *pool) {
  // todo: wait for all threads to finish
  pthread_mutex_destroy(&pool->_m);
  for (int i = 0; i < pool->thread_count; i++) {
    pthread_mutex_destroy(&pool->threads[i].mutex);
    pthread_cond_destroy(&pool->threads[i].cond);
    // todo: cleanup job queues
  }
  free(pool->threads);
}

void tpool_add_jobs(tpool_t *pool, job_queue_t jobs) {
  // require due lock
  // require thread's lock
  // due jobs
  // signal thread's cond
  // release thread's lock
  // step on due
  // release due lock
}

void request_thread_exec() {
  // spwan a few jobs
  // add jobs to pool
  // aquire result's lock
  // while no result --> wait on result's cond
  // take results
  // release result's lock
}

void calc_thread_exec() {
  // aquire self's lock
  // while no job --> wait on self's cond
  // take all jobs
  // release self's lock
  // calculate results
  // aquire result's lock
  // feed results
  // signal result's cond
  // release result's lock
}

int main() {
  // create pool
  if (0 != tpool_create(&pool, 10))
    return -1;
  // create requests
  // wait for stop
  return 0;
}