#include <pthread.h>     // pthread_create, pthread_join
#include <syslog.h>      // syslog()

void *helloThread(void *threadp)
{
    // Log the thread message at Critical debug level
    syslog(LOG_CRIT, "[COURSE:%u][ASSIGNMENT:%u] Hello World from Thread!", 1,1 );
}

int main (int argc, char *argv[])
{
    // Log the main hello message at Critical debug level
    syslog(LOG_CRIT, "[COURSE:%u][ASSIGNMENT:%u] Hello World from Main!\n", 1, 1);

    // Create and spawn a thread using the default thread attributes and whose
    // execution starts at the helloThread function. Pass no parameters to the
    // thread.
    pthread_t thread;
    pthread_create(&thread, NULL, helloThread, NULL);

    // Block until the helloThread has exited and do not store the return code
    pthread_join(thread, NULL);
}
