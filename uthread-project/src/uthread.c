#include "uthread.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

/* ── Global state ────────────────────────────────────────────────────────── */

uthread_t           *current_thread = NULL; /* thread currently on CPU */
uthread_t           *all_threads    = NULL; /* head of all-threads list */
uthread_sched_policy_t sched_policy = UTHREAD_SCHED_FIFO;
int                  next_tid       = 0;

/* ── Forward declarations ────────────────────────────────────────────────── */

/* TODO Stage 3: uthread_init   */
/* TODO Stage 3: uthread_create */
/* TODO Stage 4: uthread_yield  */
/* TODO Stage 4: uthread_exit   */
/* TODO Stage 4: uthread_start  */
/* TODO Stage 7: uthread_join   */
/* TODO Stage 7: uthread_delete */
/* TODO Stage 6: uthread_set_priority */
/* TODO Stage 7: uthread_list   */
