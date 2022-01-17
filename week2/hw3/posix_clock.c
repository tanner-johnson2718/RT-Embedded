/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/*                                                                          */
/****************************************************************************/

//*****************************************************************************
// Building, Running, and demo)
//----------------------------------------------------
// - make
// - sudo ./posix_clock
// - Console output will show clock res and stats of test
// - syslog will contain individual test results
// 
//Readability and Maintainability Improvements)
//----------------------------------------------------
// - Removing commented out and #if 0 guarded code
// - Removed end_delay_test and just inlined code
// - removed unused variables
// - Scoping variables to local context where possible
// - Removing the msec and usec from print to reduce clutter
// - Added code to alert user nanosleep failed
// - Removed floating point representatoin.
//     - Already went through hassle of using integers, why also have floats
// - Adding comments (mine use "//", existing ones use "/* */")
// - Consistent indentation
//
// Functionality Additions)
//----------------------------------------------------
// - Added compator function to test the greater than operation on timespec
// - Added sum operation for timespec
// - Logged individual test results instead of printf
// - Added functionality to get stats from test
// - printf stats instead
//
// Debugging w/ gdb)
//----------------------------------------------------
// - sudo gdb ./posix_clock
// - Not sure what to debug here
// - Something interesting to explore would be how threads, context switches,
//   cpu assignment can be debuged in GDB. Given this app is basically single
//   threaded will save that for the thread affinity code.
//
// How code can be used for realtime services)
//----------------------------------------------------
// - This code can be used to get both absolute and relative time from the OS
//   and hardware. This code provides basic primitives for accessing sleep funcs
//   to delay / poll. Moreover primitives such as delta_t are provided to
//   manipulate the underlying time data structure timespec.
//*****************************************************************************

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>

#define NSEC_PER_SEC (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_USEC (1000)

#define ERROR (-1)
#define OK (0)

// Test Parameters
#define TEST_SECONDS (0)
#define TEST_NANOSECONDS (NSEC_PER_MSEC * 10)
#define TEST_ITERATIONS (100)
#define MY_CLOCK CLOCK_MONOTONIC_RAW

void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}

// Sum t2 into t1
void sum(struct timespec *t1, struct timespec *t2)
{
  t1->tv_nsec += t2->tv_nsec;   // Cannot overflow, if valid time to begin w.

  if(t1->tv_nsec >= NSEC_PER_SEC)
  {
    t1->tv_sec  += 1;    // If valid time, then will only ever add 1 to secs
    t1->tv_nsec -= NSEC_PER_SEC;
  }
}

// Return one if t1 > t2. Else 0.
int greater(struct timespec *t1, struct timespec *t2)
{
  // More seconds in t1
  if(t1->tv_sec > t2->tv_sec)
  {
    return 1;
  }

  // More seconds in t2
  if(t1->tv_sec < t2->tv_sec)
  {
    return 0;
  }

  // Only here if same number of seconds
  return (t1->tv_nsec > t2->tv_nsec);
}

int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  // case 1 - less than a second of change
  if(dt_sec == 0)
  {

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
		  delta_t->tv_sec = 0;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
		  delta_t->tv_sec = 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means stop is earlier than start
	  {
	         printf("stop is earlier than start\n");
		 return(ERROR);  
	  }
  }

  // case 2 - more than a second of change, check for roll-over
  else if(dt_sec > 0)
  {

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
		  delta_t->tv_sec = dt_sec;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
		  delta_t->tv_sec = delta_t->tv_sec + 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means roll over
	  {
		  delta_t->tv_sec = dt_sec-1;
		  delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
	  }
  }

  return(OK);
}

void *delay_test(void *threadID)
{
  int idx, rc;
  unsigned int max_sleep_calls=3;
  int flags = 0;
  struct timespec rtclk_resolution;
  double real_dt;

  struct timespec sleep_time = {0, 0};
  struct timespec sleep_requested = {0, 0};
  struct timespec remaining_time = {0, 0};

  unsigned int sleep_count = 0;

  struct timespec rtclk_dt = {0, 0};
  struct timespec rtclk_start_time = {0, 0};
  struct timespec rtclk_stop_time = {0, 0};
  struct timespec delay_error = {0, 0};
  struct timespec max_error = {0, 0};
  struct timespec accum_error = {0,0};

  if(clock_getres(MY_CLOCK, &rtclk_resolution) == ERROR)
  {
    perror("clock_getres");
    exit(-1);
  }
  else
  {
    printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec);
  }

  for(idx=0; idx < TEST_ITERATIONS; idx++)
  {

    /* run test for defined seconds */
    sleep_time.tv_sec=TEST_SECONDS;
    sleep_time.tv_nsec=TEST_NANOSECONDS;
    sleep_requested.tv_sec=sleep_time.tv_sec;
    sleep_requested.tv_nsec=sleep_time.tv_nsec;

    /* start time stamp */ 
    clock_gettime(MY_CLOCK, &rtclk_start_time);

    /* request sleep time and repeat if time remains */
    do 
    {
      if(rc=nanosleep(&sleep_time, &remaining_time) == 0) break;
         
      sleep_time.tv_sec = remaining_time.tv_sec;
      sleep_time.tv_nsec = remaining_time.tv_nsec;
      sleep_count++;
    } 
    while(((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0))
		  && (sleep_count < max_sleep_calls));

    if(sleep_count == max_sleep_calls)
    {
      printf("ERROR called nanosleep max times without getting proper delay\n");
    }

    // ending time stamp
    clock_gettime(MY_CLOCK, &rtclk_stop_time);

    // Calculate the difference between starting and stoping time
    delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);

    // Calculate the difference between expected and actual dely
    delta_t(&rtclk_dt, &sleep_requested, &delay_error);

    syslog(LOG_CRIT, "Clock dt    : sec = %ld, nsec=%ld\n", rtclk_dt.tv_sec, rtclk_dt.tv_nsec);
    syslog(LOG_CRIT, "Clock error : sec = %ld, nsec = %ld\n", delay_error.tv_sec, delay_error.tv_nsec);
  
    // Update max error
    if(greater(&delay_error, &max_error))
    {
      max_error.tv_nsec = delay_error.tv_nsec;
      max_error.tv_sec  = delay_error.tv_sec;
    }

    // Add it to the accumulated error
    sum(&accum_error, &delay_error);
  }

  // Print stats
  printf("Max Error = %lds, %ldns\n", max_error.tv_sec, max_error.tv_nsec);

  // Note this wont work if TEST_ITERATIONS is larger or if accum error has
  // more than 1s of accumulated error, but for a demo its fine.
  printf("Avg Error = %lds, %ldns\n", accum_error.tv_sec / TEST_ITERATIONS, accum_error.tv_nsec / TEST_ITERATIONS);
}

void main(void)
{

  // Allocate heap variables to space the test thread
  pthread_t main_thread;
  pthread_attr_t main_sched_attr;
  int rt_max_prio;
  struct sched_param main_param;
  int rc;

  printf("Before adjustments to scheduling policy:\n");
  print_scheduler();

  // Populate pthread attribute struct to use SCHED_FIFO real time scheduling
  // at max priority
  main_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_init(&main_sched_attr);
  pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);
  pthread_attr_setschedparam(&main_sched_attr, &main_param);

  // Set current process / main thread to use SCHED_FIFO real time scheduling
  // at max priority.
  if ((rc = sched_setscheduler(getpid(), SCHED_FIFO, &main_param)))
  {
    printf("ERROR; sched_setscheduler rc is %d\n", rc);
    perror("sched_setschduler"); exit(-1);
  }

  printf("After adjustments to scheduling policy:\n");
  print_scheduler();

  // Launch the test thread
  if ( (rc = pthread_create(&main_thread, &main_sched_attr, delay_test,NULL)) )
  {
    printf("ERROR; pthread_create() rc is %d\n", rc);
    perror("pthread_create");
    exit(-1);
  }

  // Block until the test thread completes.
  pthread_join(main_thread, NULL);

  // Free any reserouces allocated by the attr init call above
  if(pthread_attr_destroy(&main_sched_attr) != 0)
  {
    perror("attr destroy");
  }
  
  printf("TEST COMPLETE\n");
}