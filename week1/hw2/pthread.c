#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#include <syslog.h>

#define NUM_THREADS 128

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];


void *counterThread(void *threadp)
{
    int sum=0, i, idx;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    idx = threadParams->threadIdx;

    for(i=1; i < (threadParams->threadIdx)+1; i++)
        sum=sum+i;
 
    syslog(LOG_CRIT, "[COURSE:%u][ASSIGNMENT:%u]: Thread idx=%u, sum[1...%u]=%u", 1,2, idx, idx, sum);
}


int main (int argc, char *argv[])
{

   int rc;
   int i;

   for(i=0; i < NUM_THREADS; i++)
   {
       threadParams[i].threadIdx=i;

       pthread_create(&threads[i],   // pointer to thread descriptor
                      (void *)0,     // use default attributes
                      counterThread, // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                     );

   }

   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   
}
