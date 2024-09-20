#include <linux/module.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <asm/pgtable.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#ifndef __NR_syscalls
#define __NR_syscalls (332 + 1)
#endif


MODULE_LICENSE("GPL");

static int my_sys_call_nr;
static unsigned long *sys_call_table;
static unsigned long sys_ni_syscall;
static struct proc_dir_entry *my_sys_call_nr_proc;


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
    pte_t *pte, v;

    if (!sys_call_table) {
        printk(KERN_ERR "sys_call_table not find\n");
        return -1;
    }

    pte = lookup_address((unsigned long)sys_call_table, &level);
    if (!pte) {
        printk(KERN_ERR "lookup_address sys_call_table failed\n");
        return -1;
    }
    v.pte = pte->pte;

    // if (level == PG_LEVEL_4K) {
    //     printk(KERN_INFO "leve is PG_LEVEL_4K\n");
    // } else {
    //     printk(KERN_INFO "level is %d\n", level);
    // }

    if (mk)
        v = pte_mkwrite(v);
    else
        v = pte_wrprotect(v);
    
    pte->pte = v.pte;
    
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

#define BUFLEN 100
static long pr_hello(unsigned long __user *addr)
{
    // 为进程申请一块内存，并初始化为hello world!
    struct mm_struct *mm = current->active_mm;
    unsigned long oldbrk = mm->brk;

    printk(KERN_INFO "begin to call vm_brk to incease my brk\n");

    // down_write(&mm->mmap_sem);
    if (vm_brk(oldbrk, BUFLEN) != 0) {
        // up_write(&mm->mmap_sem);
        printk(KERN_ERR "brk failed\n");
        return -1;
    }
    mm->brk += BUFLEN;
    // up_write(&mm->mmap_sem);
    // snprintf((char *)oldbrk, BUFLEN, "hello world!\n");
    char msg[] = "hello world!";
    if (copy_to_user((void *)oldbrk, (void *)msg, sizeof(msg))) {
        printk(KERN_ERR "copy message into oldbrk failed\n");
        return -1;
    }

    if (copy_to_user(addr, &oldbrk, sizeof(unsigned long))) {
        printk(KERN_ERR "copy address to user program failed\n");
        return -1;
    }
    
    printk(KERN_INFO "call vm_brk success, and return it's old brk %ld\n", oldbrk);

    return 0;
}

int sc_nr_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%d", my_sys_call_nr);
    return 0;
}

static int sc_nr_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, sc_nr_show, NULL);
}

struct file_operations sc_open = {
    .open = sc_nr_open,
    .read = seq_read,
};

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

    if (set_sys_call(my_sys_call_nr, (unsigned long)pr_hello) < 0) {
        return -1;
    }

    pr_sys_call_table();

    my_sys_call_nr_proc = proc_create("my_sys_call_nr", 0, NULL, &sc_open);
    if (!my_sys_call_nr_proc) {
        set_sys_call(my_sys_call_nr, sys_ni_syscall);
        printk(KERN_ERR "Create /proc/my_sys_call_nr failed\n");
        return -1;
    }
    
    printk(KERN_INFO "sc module initialize OK!\n");

    return 0;
}

static void __exit sc_exit(void)
{
    set_sys_call(my_sys_call_nr, sys_ni_syscall);
    remove_proc_entry("my_sys_call_nr", NULL);
    printk(KERN_WARNING"sc module exit ok!\n");
}


module_init(sc_init);
module_exit(sc_exit);



