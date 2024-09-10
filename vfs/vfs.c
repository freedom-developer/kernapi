#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/seq_file.h>


MODULE_LICENSE("GPL");

struct list_head *modules, *curmod;

void *mod_start(struct seq_file *m, loff_t *pos)
{
    printk(KERN_INFO "mod_start\n");
    return list_entry(curmod, struct module, list);
}

void mod_stop(struct seq_file *m, void *v)
{
    printk(KERN_INFO "mod_stop\n");
    struct module *mod = (struct module *)v;
    curmod = &mod->list;
}

void *mod_next(struct seq_file *m, void *v, loff_t *pos)
{
    struct module *mod = (struct module *)v;
    
    curmod = mod->list.next;

    printk(KERN_INFO "mod_next\n");

    (*pos)++;
    if (curmod == modules) {
        return NULL;
    }
    
    return list_entry(curmod, struct module, list);
}

int mod_show(struct seq_file *m, void *v)
{
    struct module *mod = (struct module *)v;
    seq_printf(m, "name: %s", mod->name);
    printk(KERN_INFO "mod_show\n");

    seq_printf(m, "\n");
    return 0;
}

struct seq_operations mod_seq_ops = {
    .start = mod_start,
    .stop = mod_stop,
    .next = mod_next,
};

int mod_open(struct inode *inode, struct file *filp)
{
    if (!(modules = (struct list_head *)kallsyms_lookup_name("modules"))) {
        printk(KERN_ERR "kernel symbol modules not found\n");
        return -ENOENT;
    }
    curmod = modules->next;
    if (curmod == modules->next) {
        printk(KERN_ERR, "No module\n");
        return -ENOENT;
    }

    printk(KERN_INFO "mod_open\n");

    return seq_open(filp, &mod_seq_ops);
}

const struct file_operations mod_fops = {
    .open = mod_open,
    .read = seq_read,
};

static int __init vfs_init(void)
{
    struct proc_dir_entry *mods;

    // 添加文件/proc/mods
    if (!(mods = proc_create("mods", 0, NULL, &mod_fops))) {
        printk(KERN_ERR "create /proc/mods file error\n");
        return -1;
    }

    return 0;
}

static void __exit vfs_exit(void)
{
    remove_proc_entry("mods", NULL);

    printk(KERN_WARNING"vfs module exit ok!\n");
}


module_init(vfs_init);
module_exit(vfs_exit);

