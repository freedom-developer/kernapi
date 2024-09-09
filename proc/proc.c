#include <linux/module.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/sched/task.h>

MODULE_LICENSE("GPL");

int my_function(void *argc)
{
    printk(KERN_INFO "in the kernel thread function!\n");
    return 0;
}

void test_task_pid_nr_ns(void)
{
    int ret;

    ret = kernel_thread(my_function, NULL, 0);
    struct pid *kpid = find_get_pid(ret);

    struct task_struct *tsk = pid_task(kpid, PIDTYPE_PID);

    pid_t nr = __task_pid_nr_ns(tsk, PIDTYPE_PID, kpid->numbers[kpid->level].ns);

    printk(KERN_INFO "kernel thread pid: %d\n", ret);
    printk(KERN_INFO "__task_pid_nr_ns pid: %d\n", nr);
    printk(KERN_INFO "current pid: %d\n", current->pid);
    printk(KERN_INFO "kernel thread pid in namespace: %d\n", kpid->numbers[kpid->level].nr);
}


static int __init proc_init(void)
{
    test_task_pid_nr_ns();
    return 0;
}

static void __exit proc_exit(void)
{
    printk(KERN_WARNING"proc exit ok!\n");
}

int a_module(void)
{

    return 0;
}


module_init(proc_init);
module_exit(proc_exit);

