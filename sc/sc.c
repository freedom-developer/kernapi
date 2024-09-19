#include <linux/module.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <asm-generic/unistd.h>
#include <asm/pgtable.h>


MODULE_LICENSE("GPL");

static int my_sys_call_nr;
static unsigned long *sys_call_table;
static unsigned long sys_ni_syscall;


static int find_sys_call_table(void)
{
    if (sys_call_table)
        return 0;

    sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");
    if (!sys_call_table) {
        printk(KERN_ERR "Symbol 'sys_call_table' not found\n");
        return -1;
    }
    return 0;
}

static int find_sys_ni_syscall(void)
{
    if (sys_ni_syscall)
        return 0;
    
    sys_ni_syscall = kallsyms_lookup_name("sys_ni_syscall");
    if (!sys_ni_syscall) {
        printk(KERN_ERR "Symbol 'sys_ni_syscall' not found\n");
        return -1;
    }
    return 0;
}

static void pr_sys_call_table(void)
{
    int i;
    char sc_name[KSYM_NAME_LEN];

    if (!sys_call_table) {
        if (find_sys_call_table() < 0)
            return;
    }

    for (i = 0; i < __NR_syscalls; i++) {
        sprint_symbol_no_offset(sc_name, sys_call_table[i]);
        printk(KERN_INFO "syscall %d: %s\n", i, sc_name);
    }
}

static int find_sys_call_nr(int start)
{
    int i, j;

    if (start >= __NR_syscalls) {
        start = start % __NR_syscalls;
    }

    if (!sys_call_table || !sys_ni_syscall) {
        printk(KERN_ERR "sys_call_table or sys_ni_syscall is empty\n");
        return -1;
    }

    for (i = start, j = 0; j < __NR_syscalls ; i++, j++) {
        if (i == __NR_syscalls) {
            i = 0;
        }
        if (sys_call_table[i] == sys_ni_syscall) {
            return i;
        }        
    }

    printk(KERN_ERR "sys_ni_syscall not found in sys_call_table\n");

    return -1;
}

static int sys_call_table_wr_mk_protect(int mk)
{
    int level;
    pte_t *pte;

    if (!sys_call_table) {
        printk(KERN_ERR "sys_call_table not find\n");
        return -1;
    }

    pte = lookup_address((unsigned long)sys_call_table, &level);
    if (!pte) {
        printk(KERN_ERR "lookup_address sys_call_table failed\n");
        return -1;
    }

    if (mk)
        pte_mkwrite(*pte);
    else
        pte_wrprotect(*pte);
    
    return 0;
}

static inline int sys_call_table_mkwrite(void)
{
    return sys_call_table_wr_mk_protect(1);
}

static inline int sys_call_table_wrprotect(void)
{
    return sys_call_table_wr_mk_protect(0);
}

static int set_sys_call(int nr, unsigned long sys_call_func)
{
    if (nr >= __NR_syscalls) {
        printk(KERN_ERR "Invalid nr %d\n", nr);
        return -1;
    }

    if (sys_call_table_mkwrite() < 0) {
        return -1;
    }

    sys_call_table[nr] = sys_call_func;

    return sys_call_table_wrprotect();
}




static int __init sc_init(void)
{
    // 初始化全局变量
    sys_call_table = NULL;
    sys_ni_syscall = 0; 
    my_sys_call_nr = -1;

    if (find_sys_call_table() < 0) {
        return -1;
    }

    if (find_sys_ni_syscall() < 0) {
        return -1;
    }

    if ((my_sys_call_nr = find_sys_call_nr(0)) < 0) {
        return -1;
    }

    return 0;
}

static void __exit sc_exit(void)
{
    printk(KERN_WARNING"sc module exit ok!\n");
}


module_init(sc_init);
module_exit(sc_exit);



