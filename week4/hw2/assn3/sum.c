#define _GNU_SOURCE
#include <pthread.h>   // pthread_*
#include <sched.h>     // sched_*
#include <syslog.h>    // syslog()
#include <stdlib.h>    // exit()
#include <stdio.h>     // printf()
#include <unistd.h>    // getpid()

#define NUM_THREADS 128
#define NUM_CPUS 4

void *counterThread(void *threadp)
{
    int sum = 0;
    int idx = *((int*) threadp);
    int i = 0;

    for(i = 0; i < idx; i++)
    {
        sum += i;
    }
    
    syslog(LOG_CRIT,"Thread idx=%d, sum[1...%d]=%d, Running on core : %d\n", 
           idx, idx, sum, sched_getcpu());
}

int main (int argc, char *argv[])
{
    // Allocate heap space for thread meta data
    int                idxs[NUM_THREADS] = {0};
    pthread_t          threads[NUM_THREADS] = {0};
    pthread_attr_t     thread_attrs[NUM_THREADS] = {0};

    // Temp variables to set pthread attributes
    struct sched_param fifo_param = {0};
    cpu_set_t cpuset = {0};

    // Set this process to use SCHED_FIFO so that main thread is RT on core 0
    // at max prio
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    if(sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset))
    {
        printf("Failed to set affinity of process, pls run as root\n");
        exit(1);
    }

    fifo_param.sched_priority = sched_get_priority_max(SCHED_FIFO);

    if(sched_setscheduler(getpid(), SCHED_FIFO, &fifo_param))
    {
        printf("Failed to set scheduler of process, pls run as root\n");
        exit(1);
    }
   
    int i;
    for(i = 0; i < NUM_THREADS; ++i)
    { 
        CPU_ZERO(&cpuset);
        CPU_SET(i % NUM_CPUS, &cpuset);

        idxs[i] = i;

        // Set each thread to run RT w/ SCHED_FIFO at max prio and on the CPU
        // of idx % NUMCPUS.
        pthread_attr_init(thread_attrs + i);
        pthread_attr_setinheritsched(thread_attrs + i, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(thread_attrs + i, SCHED_FIFO);
        pthread_attr_setaffinity_np(thread_attrs + i, sizeof(cpu_set_t), &cpuset);
        pthread_attr_setschedparam(thread_attrs + i, &fifo_param);

        // Spawn each thread to execute the counterThread function
        pthread_create(threads + i,  
                       thread_attrs + i, 
                       counterThread,
                       idxs + i
                     );
    }

    // Block until all threads have finished executing
    for(i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
}
