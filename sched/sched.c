#include <linux/module.h>
#include <linux/init.h>
#include <kernel/kthread.h>


MODULE_LICENSE("GPL");

int my_function(void *data)
{
    printk(KERN_INFO "task name: %s\n", current->comm);
    printk(KERN_INFO "task pid: %d\n", current->pid);
    printk(KERN_INFO "task tgid: %d\n", current->tgid);
}

static int __init sched_init(void)
{
    
    return 0;
}

static void __exit sched_exit(void)
{
    printk(KERN_WARNING"proc exit ok!\n");
}

module_init(sched_init);
module_exit(sched_exit);

