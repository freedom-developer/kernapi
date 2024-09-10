#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/fs.h>


MODULE_LICENSE("GPL");

struct list_head *modules;
struct module *mod;

struct module *mod_start(struct seq_file *m, loff_t *pos)
{
    *pos = 0;
    if (!(modules = (struct list_head *)kallsyms_lookup_name("modules"))) {
        printk(KERN_ERR "kernel symbol modules not found\n");
        return NULL;
    }
    mod = list_entry(modules, struct module, list);
    return mod;
}



struct seq_operations mod_seq_ops = {
    .start = mod_start,
    .stop = 
}

int mod_open(struct inode *inode, struct file *filp)
{
    return seq_open(filp, &mod_seq_ops);
}

const struct file_operations mod_fops = {
    .open = mod_open,
    .read = seq_read,
};

static int __init vfs_init(void)
{
    struct module *mod;
    struct list_head *modules;

    modules = (struct list_head *)kallsyms_lookup_name("modules");
    if (!modules) {
        printk(KERN_ERR "kernel symbol (modules) not found\n");
        return -1;
    }

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

