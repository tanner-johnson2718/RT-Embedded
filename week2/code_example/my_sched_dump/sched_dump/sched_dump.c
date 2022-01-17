#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>           // task_struct
#include <linux/sched/signal.h>    // for_each_process

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Process Explorer from kernel space");
MODULE_AUTHOR("me");
MODULE_VERSION("0.1");

static void sched_dump()
{
    struct sched_class *class;
    for_each_class(class)
    {
        
    }
    
}

static void kexit(void) {
    return;
}

// INIT module. 
static int kinit(void) {
    
    sched_dump();

    return 0;
}

module_init(kinit);
module_exit(kexit);
