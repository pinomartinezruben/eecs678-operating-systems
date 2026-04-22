/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
  int job_number;
  int arrival_time;
  int running_time;
  int remaining_time;      /* ADDED: tracks time left; used by PSJF */
  int priority;
  int core_id;             /* -1 = not currently running on any core */
  int start_time;          /* -1 = has never gotten a CPU cycle yet */
  int finish_time;
  int last_scheduled_time; /* time this job was last placed onto a core */
} job_t;


static int       num_cores;
static scheme_t  current_scheme;
static job_t   **core_job_ptrs;  /* pointers to the job_t running on each core */
static priqueue_t job_queue;     /* the ready queue */

static float total_wait_time;
static float total_turnaround_time;
static float total_response_time;
static int   total_jobs_finished;


static int compare_fcfs(const void *a, const void *b)
{
  const job_t *ja = (const job_t *)a;
  const job_t *jb = (const job_t *)b;
  return ja->arrival_time - jb->arrival_time;
}

/* ADDED: SJF / PSJF comparer — order by remaining_time ascending.
   For SJF, remaining_time == running_time (never partially run).
   For PSJF, remaining_time is decremented as the job runs.
   Tie-break: earlier arrival time wins (per spec). */
static int compare_sjf(const void *a, const void *b)
{
  const job_t *ja = (const job_t *)a;
  const job_t *jb = (const job_t *)b;
  if (ja->remaining_time != jb->remaining_time)
    return ja->remaining_time - jb->remaining_time;
  return ja->arrival_time - jb->arrival_time;
}

static int compare_pri(const void *a, const void *b)
{
  const job_t *ja = (const job_t *)a;
  const job_t *jb = (const job_t *)b;
  if (ja->priority != jb->priority)
    return ja->priority - jb->priority;
  return ja->arrival_time - jb->arrival_time;
}

static int compare_rr(const void *a, const void *b)
{
  const job_t *ja = (const job_t *)a;
  const job_t *jb = (const job_t *)b;
  return ja->arrival_time - jb->arrival_time;
}

static int find_idle_core(void)
{
  for (int i = 0; i < num_cores; i++)
    if (core_job_ptrs[i] == NULL)
      return i;
  return -1;
}

static void place_job_on_core(job_t *job, int core_id, int time)
{
  core_job_ptrs[core_id] = job;
  job->core_id = core_id;
  job->last_scheduled_time = time;

  /* Record the very first time this job gets a CPU cycle (response time). */
  if (job->start_time == -1)
    job->start_time = time;
}

static void update_remaining_times(int time)
{
  for (int i = 0; i < num_cores; i++)
  {
    if (core_job_ptrs[i] != NULL)
    {
      job_t *j = core_job_ptrs[i];
      int elapsed = time - j->last_scheduled_time;
      j->remaining_time -= elapsed;
      j->last_scheduled_time = time; /* reset so we don't double-subtract */
    }
  }
}

static void accumulate_stats(job_t *job)
{
  float turnaround = (float)(job->finish_time - job->arrival_time);
  float response   = (float)(job->start_time  - job->arrival_time);
  float wait       = turnaround - (float)job->running_time;

  total_turnaround_time += turnaround;
  total_response_time   += response;
  total_wait_time       += wait;
  total_jobs_finished++;
}


/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{

  num_cores      = cores;
  current_scheme = scheme;

  total_wait_time       = 0.0f;
  total_turnaround_time = 0.0f;
  total_response_time   = 0.0f;
  total_jobs_finished   = 0;

  core_job_ptrs = malloc(sizeof(job_t *) * cores);
  for (int i = 0; i < cores; i++)
    core_job_ptrs[i] = NULL;

  switch (scheme)
  {
    case FCFS:
      priqueue_init(&job_queue, compare_fcfs);
      break;
    case SJF:
    case PSJF:
      priqueue_init(&job_queue, compare_sjf);
      break;
    case PRI:
    case PPRI:
      priqueue_init(&job_queue, compare_pri);
      break;
    case RR:
    default:
      priqueue_init(&job_queue, compare_rr);
      break;
  }
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumption:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  job_t *new_job = malloc(sizeof(job_t));
  new_job->job_number = job_number;
  new_job->arrival_time = time;
  new_job->running_time = running_time;
  new_job->remaining_time = running_time;
  new_job->priority = priority;
  new_job->core_id = -1;
  new_job->start_time = -1;
  new_job->finish_time = -1;
  new_job->last_scheduled_time = time;

  /* FIX: update remaining times before ANY comparison */
  update_remaining_times(time);

  int idle_core = find_idle_core();
  if (idle_core != -1)
  {
    place_job_on_core(new_job, idle_core, time);
    return idle_core;
  }

  if (current_scheme == PSJF || current_scheme == PPRI)
  {
    int worst_core = -1;
    job_t *worst_job = NULL;

    for (int i = 0; i < num_cores; i++)
    {
      job_t *running = core_job_ptrs[i];
      if (running == NULL) continue;

      if (worst_job == NULL)
      {
        worst_job = running;
        worst_core = i;
      }
      else
      {
        int cmp_result = (current_scheme == PSJF)
          ? compare_sjf(running, worst_job)
          : compare_pri(running, worst_job);

        if (cmp_result > 0)
        {
          worst_job = running;
          worst_core = i;
        }
      }
    }

    if (worst_job != NULL)
    {
      int cmp_result = (current_scheme == PSJF)
        ? compare_sjf(new_job, worst_job)
        : compare_pri(new_job, worst_job);

      if (cmp_result < 0)
      {
        worst_job->core_id = -1;
        priqueue_offer(&job_queue, worst_job);

        place_job_on_core(new_job, worst_core, time);
        return worst_core;
      }
    }
  }

  priqueue_offer(&job_queue, new_job);
  return -1;
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  job_t *finished = core_job_ptrs[core_id];

  /* FIX: update remaining time before finishing */
  int elapsed = time - finished->last_scheduled_time;
  finished->remaining_time -= elapsed;
  finished->last_scheduled_time = time;

  finished->finish_time = time;
  accumulate_stats(finished);

  free(finished);
  core_job_ptrs[core_id] = NULL;

  job_t *next_job = (job_t *)priqueue_poll(&job_queue);
  if (next_job == NULL)
    return -1;

  place_job_on_core(next_job, core_id, time);
  return next_job->job_number;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
  job_t *expired = core_job_ptrs[core_id];

  /* FIX: update remaining time */
  int elapsed = time - expired->last_scheduled_time;
  expired->remaining_time -= elapsed;
  expired->last_scheduled_time = time;

  core_job_ptrs[core_id] = NULL;
  expired->core_id = -1;

  /* FIX: no arrival_time hack */
  priqueue_offer(&job_queue, expired);

  job_t *next_job = (job_t *)priqueue_poll(&job_queue);
  if (next_job == NULL)
    return -1;

  place_job_on_core(next_job, core_id, time);
  return next_job->job_number;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
  /* ADDED: Guard against division by zero if somehow no jobs ran. */
  if (total_jobs_finished == 0) return 0.0f;
  return total_wait_time / (float)total_jobs_finished;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
  if (total_jobs_finished == 0) return 0.0f;
  return total_turnaround_time / (float)total_jobs_finished;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
  if (total_jobs_finished == 0) return 0.0f;
  return total_response_time / (float)total_jobs_finished;
}


/**
  Free any memory associated with your scheduler.
 
  Assumption:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
  for (int i = 0; i < num_cores; i++)
  {
    if (core_job_ptrs[i] != NULL)
    {
      free(core_job_ptrs[i]);
      core_job_ptrs[i] = NULL;
    }
  }

  job_t *leftover;
  while ((leftover = (job_t *)priqueue_poll(&job_queue)) != NULL)
    free(leftover);

  priqueue_destroy(&job_queue);
  free(core_job_ptrs);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
  int size = priqueue_size(&job_queue);
  for (int i = 0; i < size; i++)
  {
    job_t *j = (job_t *)priqueue_at(&job_queue, i);
    printf("%d(%d) ", j->job_number, j->core_id);
  }
  for (int c = 0; c < num_cores; c++)
  {
    if (core_job_ptrs[c] != NULL)
      printf("%d(%d) ", core_job_ptrs[c]->job_number, c);
  }
  printf("\n");
}