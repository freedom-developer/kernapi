#include <linux/module.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <asm-generic/unistd.h>

#include <asm/pgtable.h>


MODULE_LICENSE("GPL");

static unsigned long my_sys_call_nr;
static unsigned long *sys_call_table;
static unsigned long sys_ni_syscall;

// 定义系统调用


static int __init sc_init(void)
{
    int i, level;
    // char sc_name[KSYM_NAME_LEN];

    sys_call_table = NULL;
    sys_ni_syscall = 0; 
    
    sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");
    if (!sys_call_table) {
        printk(KERN_ERR "Symbol 'sys_call_table' not found\n");
        return -1;
    }

    sys_ni_syscall = kallsyms_lookup_name("sys_ni_syscall");
    if (!sys_ni_syscall) {
        printk(KERN_ERR "Symbol 'sys_ni_syscall' not found\n");
        return -1;
    }

    /*
    for (i = 0; i < __NR_syscalls; i++) {
        sprint_symbol(sc_name, sys_call_table[i]);
        printk(KERN_INFO "%d: %s\n", i, sc_name);
    }
    */

    my_sys_call_nr = 0;
    for (i = 0; i < __NR_syscalls; i++) {
        if (sys_ni_syscall == sys_call_table[i]) {
            my_sys_call_nr = i;
            printk(KERN_INFO "Create a system call on number %ld\n", my_sys_call_nr);
            break;
        }
    }

    if (my_sys_call_nr == 0) {
        printk(KERN_ERR "sys_ni_syscall not found\n");
        return -1;
    }

    // 获取sys_call_table表项
    pte_t *pte = lookup_address((unsigned long)sys_call_table, &level);
    if (!pte) {
        printk(KERN_ERR "lookup_address sys_call_table failed\n");
        return -1;
    }

    if (!pte_write(*pte)) {
        pte_mkwrite(*pte);
    }

    // 修改sys_call_table[my_sys_call_nr]

    return 0;
}

static void __exit sc_exit(void)
{
    if (sys_call_table) {
        int level;
        pte_t *pte = lookup_address((unsigned long)sys_call_table, &level);
        if (!pte) {
            printk(KERN_ERR "sc exit: lookup_address sys_call_table failed\n");
            return;
        }
        if (pte_write(*pte)) {
            pte_wrprotect(*pte);
        }
    }
    printk(KERN_WARNING"sc module exit ok!\n");
}


module_init(sc_init);
module_exit(sc_exit);



