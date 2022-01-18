// Generic sequencer assignment. Based on seqgen3.c from class example.

// 3 Services + 1 sequencer, will use the following CPU affinitity and priority
// based on shortest period gets highest priority. The sequencer will be ran on
// CPU 0 so as not to interfere w/ the services. All services will be ran on
// the same cpu so that we can see higher priority services preempting lower
// priority services. The period of the sequencer is set to the GCD of 10ms.
// 
// |-------------------------------------------------|
// | Service |   Period    |  Capacity  | CPU | prio |
// |---------|-------------|------------|-----|------|
// |  Seq    | T_S = 10ms  |    N/A     |  0  |  99  |
// |   S1    | T_1 = 20ms  | C_1 = 10ms |  1  |  98  |
// |   S2    | T_2 = 100ms | C_2 = 10ms |  1  |  97  |
// |   S1    | T_3 = 150ms | C_3 = 20ms |  1  |  96  |
// |-------------------------------------------------|

#define _GNU_SOURCE

#include <stdio.h>        // printf()
#include <syslog.h>       // syslog()
#include <pthread.h>      // pthread_*
#include <sched.h>        // cpu_set_t, CPU macros, sched_param
#include <semaphore.h>    // sem_t, sem_wait, sem_post
#include <stdlib.h>       // exit
#include <signal.h>       // signal
#include <time.h>         // timer_create, timer_settime
#include <unistd.h>       // pause 
 
#define NANOSEC_PER_SEC (1000000000)
#define NANOSEC_PER_MSEC (1000000)
#define NUM_CPU_CORES (4)
#define NUM_THREADS (3)
#define MY_CLOCK CLOCK_MONOTONIC_RAW
#define SEQ_CPU 0
#define SERVICE_CPU 1
#define NUM_TICKS 30    // 30 10ms ticks

#define CATCH(f) if(f) {printf("ERROR %s:%d\n", __FILE__, __LINE__); exit(-1);}

#define ABS(x) x < 0 ? -x : x
#define EPS 1000

int abortTest=0;
int abortS1=0, abortS2=0, abortS3=0;
sem_t semS1, semS2, semS3;
static timer_t timer_1;
struct timespec starting_tick;
int num_1ms_spin = 0;

//*****************************************************************************
// Helper functions
//*****************************************************************************

static inline void tick(struct timespec *t1)
{
    clock_gettime(MY_CLOCK, t1);
}

void tock(struct timespec *t1)
{
    struct timespec _t2;
    struct timespec *t2 = &_t2;
    clock_gettime(MY_CLOCK, t2);

    t2->tv_sec  -= t1->tv_sec;   // strictly increasing so good
    t2->tv_nsec -= t1->tv_nsec;  // Could be negative, can't overflow

    // copy over
    t1->tv_sec = t2->tv_sec;
    t1->tv_nsec = t2->tv_nsec;

    if(t1->tv_nsec < 0)
    {
        t1->tv_nsec += NANOSEC_PER_SEC;
        t1->tv_sec -= 1;
    }
}

// t2 into t1
void copy(struct timespec *t1, struct timespec *t2)
{
    t1->tv_sec = t2->tv_sec;
    t1->tv_nsec = t2->tv_nsec;
}

void log_time(char* s)
{
    struct timespec current_time_val;
    copy(&current_time_val, &starting_tick);
    tock(&current_time_val);

    double ms = (((double) current_time_val.tv_sec)  *1000.0) 
              + (((double) current_time_val.tv_nsec) / 1000000.0);

    syslog(LOG_CRIT, "[%2.6lfms] %s", ms, s);
}

// spin for slightly under 1ms
static inline void spin_1ms()
{
    int i;
    int sum = 0;
    for(i = 0; i < num_1ms_spin; ++i)
    {
        sum += i;
    }
}

// spin the CPU for specified number of ms
void spin(int ms, int service)
{
    int i;
    char str[32];
    sprintf(str, "Service %d computing", service);
    for(i =0; i < ms; ++i)
    {
        spin_1ms();
        log_time(str);
    }
    
}

// Find the number of iterations it takes to for spin_1ms to actually spin for
// 1ms. Our initial guess will assume a 1Ghz processor and 1 loop per clock
// cycle which with pipelinig an branch prediction isn't such a bad guess. But
// should be safely over so that we can decrement the number of iterations.
void tune_spin_1ms()
{
    num_1ms_spin = NANOSEC_PER_MSEC;
    struct timespec t1 = {0,0};
    do
    {   
        tick(&t1);
        spin_1ms();
        tock(&t1);

        // syslog(LOG_CRIT, "Calibarating: %ld", t1.tv_nsec);

        num_1ms_spin -= 1000;
    } while(t1.tv_sec != 0 && ABS(t1.tv_nsec - NANOSEC_PER_MSEC) > EPS );

    
    // adjust so spin_1ms is like .8ms
    num_1ms_spin = (int) ((double)num_1ms_spin * .80);
}

//*****************************************************************************
// Service functions
//*****************************************************************************

// 20ms period w/ 10ms capacity
void* service_1(void *threadp)
{
    while(1)
    {
        sem_wait(&semS1);
        if(abortS1)
        {
            break;
        }
        log_time("Service 1 running");

        spin(10, 1);

        log_time("Service 1 yielding");
    }

    return NULL;
}

// 100ms period w/ 10ms capacity
void* service_2(void *threadp)
{
    while(1)
    {
        sem_wait(&semS2);
        if(abortS2)
        {
            break;
        }

        log_time("Service 2 running");

        spin(10, 2);

        log_time("Service 2 yielding");
    }

    return NULL;
}

// 150ms period w/ 20ms capacity
void* service_3(void *threadp)
{
    while(1)
    {
        sem_wait(&semS3);
        if(abortS3)
        {
            break;
        }

        log_time("Service 3 running");

        spin(20, 3);

        log_time("Service 3 yielding");
    }

    return NULL;
}

//*****************************************************************************
// Sequencer functions
//*****************************************************************************

// Sequencer call back handler. Called every 10ms by timer
void sequencer_handler()
{
    static int ticks = 0;

    // start clock
    if(ticks == 0)
    {
        tick(&starting_tick);
        log_time("Simulation starting");
    }
    
    // check if we should abort test
    if(ticks == NUM_TICKS)
    {
        log_time("Simulation ending");
        abortTest = 1;
        abortS1 = 1;
        abortS2 = 1;
        abortS3 = 1;

        sem_post(&semS1);
        sem_post(&semS2);
        sem_post(&semS3);

        // Disarm timer
        timer_delete(timer_1);

        // Exit sequencer thread
        pthread_exit(NULL);
    }

    // dispatch service 1 at 20ms period
    if(ticks % 2 == 0)
    {
        sem_post(&semS1);
    }

    // dispatch service 2 at 100ms period
    if(ticks % 10 == 0)
    {
        sem_post(&semS2);
    }

    // dispatch service 3 at 150ms period
    if(ticks % 15 == 0)
    {
        sem_post(&semS3);
    }

    ticks++;
}

// Sequencer starter. Will set up timer handler and simply wait for the seq
// handler to indicate that the test is done, polling every second. Also call
// tune_1ms so we can calculate how many iterations of the basic spin_1ms we
// need to do to ensure this function actually takes 1ms to execute
void* sequencer_starter()
{
    tune_spin_1ms();

    static struct itimerspec itime = {{1,0}, {1,0}};
    static struct itimerspec last_itime;

    /* set up to signal SIGALRM if timer expires */
    CATCH ( timer_create(CLOCK_REALTIME, NULL, &timer_1) )

    // Map handler to SIGALRM
    signal(SIGALRM, (void(*)()) sequencer_handler);

    // Set timer frequency to 10ms
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = (10)*NANOSEC_PER_MSEC;
    
    // set init delay to 1s to be certain all services are waiting on sem
    itime.it_value.tv_sec = 1;
    itime.it_value.tv_nsec = 0;

    // Kick off timer
    CATCH ( timer_settime(timer_1, 0, &itime, &last_itime) )

    // shouldn't get here, seq thread exits in seq handler
    while(!abortTest)
    {
        sleep(1);
    }

    return NULL;
}

//*****************************************************************************
// Entry
//*****************************************************************************

int main()
{
    // Allocate heap space for thread data
    pthread_t seq_thread;
    pthread_t service_threads[NUM_THREADS];
    pthread_attr_t seq_attr;
    pthread_attr_t service_attr[NUM_THREADS];
    struct sched_param seq_param;
    struct sched_param service_param[NUM_THREADS];

    // Allocate and set cpu masks
    cpu_set_t service_cpuset;
    cpu_set_t seq_cpuset;
    CPU_ZERO(&service_cpuset);
    CPU_ZERO(&seq_cpuset);
    CPU_SET(SERVICE_CPU, &service_cpuset);
    CPU_SET(SEQ_CPU, &seq_cpuset);
    
    // Init semaphores
    if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }

    // Init sequencer pthread attrs
    seq_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    CATCH ( pthread_attr_init(&seq_attr) )
    CATCH ( pthread_attr_setinheritsched(&seq_attr, PTHREAD_EXPLICIT_SCHED) )
    CATCH ( pthread_attr_setschedpolicy(&seq_attr, SCHED_FIFO) )
    CATCH ( pthread_attr_setaffinity_np(&seq_attr, sizeof(cpu_set_t), &seq_cpuset) )
    CATCH ( pthread_attr_setschedparam(&seq_attr, &seq_param) )

    // Init service threads
    int i;
    for(i = 0; i < NUM_THREADS; ++i)
    {
        service_param[i].sched_priority = sched_get_priority_max(SCHED_FIFO) - 1 - i;
        CATCH ( pthread_attr_init(service_attr+i) )
        CATCH ( pthread_attr_setinheritsched(service_attr+i, PTHREAD_EXPLICIT_SCHED) )
        CATCH ( pthread_attr_setschedpolicy(service_attr+i, SCHED_FIFO) )
        CATCH ( pthread_attr_setaffinity_np(service_attr+i, sizeof(cpu_set_t), &service_cpuset) )
        CATCH ( pthread_attr_setschedparam(service_attr+i, service_param+i) )
    }

    // spawn service threads
    CATCH ( pthread_create(service_threads+0, service_attr+0, service_1, NULL) )
    CATCH ( pthread_create(service_threads+1, service_attr+1, service_2, NULL) )
    CATCH ( pthread_create(service_threads+2, service_attr+2, service_3, NULL) )

    // Spawn sequencer
    CATCH ( pthread_create(&seq_thread, service_attr+2, sequencer_starter, NULL) )

    // Main thread just blocks while sequencer drives the scheduling. When seq
    // exits, the test is over. Join on service to double check they died.
    CATCH ( pthread_join(seq_thread, NULL) )
    CATCH ( pthread_join(service_threads[0], NULL) )
    CATCH ( pthread_join(service_threads[1], NULL) )
    CATCH ( pthread_join(service_threads[2], NULL) )

    // Clean up pthread attrs
    CATCH ( pthread_attr_destroy(&seq_attr) )
    CATCH ( pthread_attr_destroy(service_attr + 0) )
    CATCH ( pthread_attr_destroy(service_attr + 1) )
    CATCH ( pthread_attr_destroy(service_attr + 2) )

    // Proper exit
    return 0;
}