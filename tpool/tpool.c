#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// #define CALC_LOG
#define REQST_LOG

typedef enum OP { ADD, SUB, MUT, DIV } OP;

static char op_char_map[] = {'+', '-', '*', '/'};

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
  int due;
  pthread_mutex_t _m; // for protecting _m among request threads
  ithread_t *threads;
} tpool_t;

typedef struct calc_thread_arg_t {
  int thread_index;
} calc_thread_arg_t;

void calc_thread_exec(calc_thread_arg_t *calc_thread_arg);

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
  printf("init pool for %d threads.\n", num_threads);
  pool->thread_count = num_threads;
  calc_thread_arg_t *args[num_threads];

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
    args[i] = (calc_thread_arg_t *)malloc(sizeof(calc_thread_arg_t));
    args[i]->thread_index = i;
#ifdef CALC_LOG
    printf("create thread %d\n", i);
#endif
    pthread_create(&pool->threads[i].thread, NULL, (void *)calc_thread_exec,
                   (void *)args[i]);
  }
  return 0;

cleanup_ithreads:
  free(pool->threads);
return_error:
  return -1;
}

/**
 * @brief deallocate all memories inside `pool`
 *
 * @param pool
 */
void tpool_destroy(tpool_t *pool) {
  // todo: wait for all threads to finish
  job_t *pj, *qj;
  pthread_mutex_destroy(&pool->_m);
  for (int i = 0; i < pool->thread_count; i++) {
    pthread_mutex_destroy(&pool->threads[i].mutex);
    pthread_cond_destroy(&pool->threads[i].cond);
    pj = pool->threads[i].job_queue.head;
    while (pj != NULL) {
      qj = pj;
      pj = pj->next;
      free(qj);
    }
  }
  free(pool->threads);
}

/**
 * @brief append jobs in `jobs` to next on-due thread's job queue
 *
 * @param pool
 * @param jobs
 */
void tpool_add_jobs(tpool_t *pool, job_queue_t jobs) {
  pthread_mutex_lock(&pool->_m);
  pthread_mutex_lock(&pool->threads[pool->due].mutex);
  if (0 == pool->threads[pool->due].job_queue.job_count) {
    pool->threads[pool->due].job_queue.head = jobs.head;
    pool->threads[pool->due].job_queue.tail = jobs.tail;
    pool->threads[pool->due].job_queue.job_count = jobs.job_count;
  } else {
    pool->threads[pool->due].job_queue.tail->next = jobs.head;
    pool->threads[pool->due].job_queue.job_count += jobs.job_count;
  }
#ifdef CALC_LOG
  printf("thread %d: current job count:%d\n", pool->due,
         pool->threads[pool->due].job_queue.job_count);
#endif
  pthread_cond_signal(&pool->threads[pool->due].cond);
  pthread_mutex_unlock(&pool->threads[pool->due].mutex);
  pool->due = (pool->due + 1) % pool->thread_count;
  pthread_mutex_unlock(&pool->_m);
}

void request_thread_exec() {
  // spwan 1~10 jobs
  job_queue_t jobs;
  job_t *pj, *qj;
  int job_count = rand() % 10 + 1;
  pj = (job_t *)malloc(sizeof(job_t));
  pj->a = rand() % 100;
  pj->b = rand() % 100;
  pj->op = ADD + rand() % 4;
  pj->next = NULL;
  jobs.head = jobs.tail = pj;
  for (int i = 0; i < job_count - 1; i++) {
    qj = pj;
    pj = (job_t *)malloc(sizeof(job_t));
    pj->a = rand() % 100;
    pj->b = rand() % 100;
    pj->op = ADD + rand() % 4;
    jobs.tail = pj;
    pj->next = NULL;
    qj->next = pj;
  }
  jobs.job_count = job_count;
#ifdef REQST_LOG
  printf("spawn %d jobs\n", job_count);
#endif
  tpool_add_jobs(&pool, jobs);
  // todo:
  // aquire result's lock
  // while no result --> wait on result's cond
  // take results
  // release result's lock
}

void calc_thread_exec(calc_thread_arg_t *args) {
  int me = args->thread_index;
  free(args);
  pthread_mutex_lock(&pool.threads[me].mutex);
#ifdef CALC_LOG
  printf("thread %d waiting\n", me);
#endif
  while (pool.threads[me].job_queue.job_count == 0) {
    pthread_cond_wait(&pool.threads[me].cond, &pool.threads[me].mutex);
  }
  // take all jobs
  job_queue_t jobs;
  jobs.head = pool.threads[me].job_queue.head;
  jobs.tail = pool.threads[me].job_queue.tail;
  jobs.job_count = pool.threads[me].job_queue.job_count;
  pool.threads[me].job_queue.tail = NULL;
  pool.threads[me].job_queue.head = NULL;
  pool.threads[me].job_queue.job_count = 0;
  pthread_mutex_unlock(&pool.threads[me].mutex);
#ifdef CALC_LOG
  printf("thread %d jobs: %d\n", me, jobs.job_count);
#endif

  int results[jobs.job_count];
  job_t *pj = jobs.head;
  job_t *qj;
  for (int i = 0; i < jobs.job_count; i++) {
    results[i] = calc(pj->a, pj->b, pj->op);
    pj = pj->next;
  }

  pj = jobs.head;
  for (int i = 0; i < jobs.job_count; i++) {
    printf("%d %c %d = %d\n", pj->a, op_char_map[pj->op], pj->b, results[i]);
    qj = pj;
    pj = pj->next;
    free(qj);
  }
  // todo:
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
  pthread_t request_threads[10];
  for (int i = 0; i < 10; i++) {
    pthread_create(&request_threads[i], NULL, (void *)request_thread_exec,
                   NULL);
  }
  // todo: wait for stop
  return 0;
}