#include "uthread.h"

/* Optional: preemptive RR via SIGALRM + setitimer.
   Implement only after the cooperative scheduler is stable.

   TODO (optional Stage 5 extension):
     timer_init(int interval_ms)   — arm setitimer
     timer_stop()                  — disarm setitimer
     sigalrm_handler()             — signal handler that calls schedule()
*/
