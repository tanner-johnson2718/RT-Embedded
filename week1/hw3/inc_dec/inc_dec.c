// The inc_dec program will spawn 2 threads, one to increment an unprotected
// global value and one to decrement it. Each thread, inc / dec, runs for the
// same number of iterations. The main thread that spawned the inc and dec
// thread simply waits for the two threads to complete and then exits.

// This program explifies two behaviors
// 1) A race condition on the unprotected global
//     -> Since inc and dec run the same number of iterations, one would
//        expect that the final value of gsum is 0.
//     -> This is not the case given we run the program enough times or 
//        equivalently set the # of iterations high enough
//     -> Suppose inc runs and it loads the value of gsum from RAM int reg A
//     -> For example say gsum = 0, so inc sets reg A = 0
//     -> Its then interrupted and its reg state is saved
//     -> Now dec run and it loads the same value inc did into reg A, reg A = 0
//     -> dec decrements the value and writes it to gsum in RAM
//     -> So gsum in RAM now = -1.
//     -> Now inc runs and its reg state is returned so that reg A = 0.
//     -> i.e. it still thinks gsum is 0.
//     -> It incs reg A to 1 and writes the value of 1 to gsum in RAM.
//     -> So if only one iteration was ran, gsum would end up with the value 1
//        not 0 as expected.
//     -> Increment and Decrement are not atomic operations and interruptions
//        of these operations lead to race condtions as explained.
//     -> To see this, set COUNT to large value i.e. 2^20, but not so large as
//        to overflow the sum
//     -> Turn off PRINT_IN_THREAD so there aren't prints every iteration
//     -> Observe the value of gsum after inc and dec exit
//     -> Will be non zero with high probability.
//   
// 2) The undeterministic behavior of the linux CFS scheduler (SCHED_OTHER)
//     -> Set COUNT to low value and turn on PRINT_IN_THREAD
//     -> Run the program
//     -> You will see that inc and dec are scheduled for execution almost
//        randomly and there appears to be no pattern by which one takes 
//        execution and for how many iterations it will be ran.
//     -> This explifies the nondeterminish of the SCHED_OTHER CFS scheduler.
//

#include <pthread.h>  // pthread_create, pthread_join
#include <stdio.h>    // printf
#include <stdlib.h>   // exit

// Set the count s.t. gsum will not overflow in the worst case i.e. where inc
// or dec is ran to completion followed by the other without any interleaving.
// So we need | gsum | < 2^63 for a signed 64 bit gsum counter.
// The worst case value is (COUNT+1)(COUNT)/2 < (COUNT)^2
// If we set COUNT = 2^20 = (1024*1024),
// then we can bound the worst value to 2^40.
// This gives us a lot of iterations, 2^20, but ensures no overflow behavior.
#define COUNT  (1024)*(1024)

// Unsafe global counter. 
long long gsum=0;

// Whether or not to print in the inc/dec thread the partial sums on each iter.
// Turn on to see the undeterministic behavior of SCHED_OTHER w/ low COUNT
// Turn off to see speed up execution when COUNT is large to see race condition
#define PRINT_IN_THREAD 0

// Simply increment the global sum in a loop
void *incThread(void *threadp)
{
    int i;
    int idx = *((int*)threadp);

    for(i=0; i<COUNT; i++)
    {
        gsum += (long long) i;

        // Will on compile in the print statement if macro is set
        #if PRINT_IN_THREAD
            printf("Increment thread idx=%d, iter=%d, gsum=%lld\n", idx, i, gsum);
        #endif
    }
}

// Simply decrement the global sum in a loop
void *decThread(void *threadp)
{
    int i;
    int idx = *((int*)threadp);

    for(i=0; i<COUNT; i++)
    {
        gsum -= (long) i;

        // Will on compile in the print statement if macro is set
        #if PRINT_IN_THREAD
           printf("Decrement thread idx=%d, iter=%d, gsum=%lld\n", idx, i, gsum);
        #endif
    }
}

int main (int argc, char *argv[])
{
    // Just check if machine is 64bit. If its not gsum will overflow.
    if(sizeof(gsum) != 8)
    {
        printf("Warning, not running on 64bit machine!\n");
        exit(1);
    }

   pthread_t threads[2];
   int id0 = 0;
   int id1 = 1;

   // Spawn the inc / dec threads using the SCHED_OTHER scheduler
   pthread_create(&threads[0], NULL, incThread, (void *)&id0);
   pthread_create(&threads[1], NULL, decThread, (void *)&id1);
   
   // Wait on them to exit
   pthread_join(threads[0], NULL);
   pthread_join(threads[1], NULL);

   // Print the final count. Will = 0 if nothing bad happens. If race cond is
   // triggered then this will be non zero
   printf("TEST COMPLETE: gsum=%lld\n", gsum);
}
