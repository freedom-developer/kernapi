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
#include <linux/slab.h>


MODULE_LICENSE("GPL");

struct mod_res {
    struct list_head *head;
    struct module *mod;
};

char *test[3] = {
    "abc",
    "def",
    "ABC"
};

void *mod_start(struct seq_file *m, loff_t *pos)
{
    struct mod_res *mod_res = (struct mod_res *)m->private;
    if (!mod_res || !mod_res->head || !mod_res->mod)
        return NULL;
    if (&mod_res->mod->list == mod_res->head)
        return NULL;
    
    return mod_res->mod;
}

void mod_stop(struct seq_file *m, void *v)
{
    // printk(KERN_INFO "mod_stop\n");
    // struct module *mod = (struct module *)v;
    // curmod = &mod->list;
}

void *mod_next(struct seq_file *m, void *v, loff_t *pos)
{
    struct mod_res *mod_res = (struct mod_res *)m->private;

    if (!mod_res || !mod_res->head || !mod_res->mod) 
        return NULL;

    (*pos)++;
    mod_res->mod = list_entry(mod_res->mod->list.next, struct module, list);
    if (&mod_res->mod->list == mod_res->head) {
        return NULL;
    }
    
    return mod_res->mod;
}

int mod_show(struct seq_file *m, void *v)
{
    struct module *mod = (struct module *)v;

    seq_printf(m, "name: %s, ", mod->name);
    seq_printf(m, "state: %d, ", mod->state);
    seq_printf(m, "path: /sys%s, ", kobject_get_path(&mod->mkobj.kobj, GFP_KERNEL));

    
    seq_printf(m, "\n");
    
    return 0;
}

struct seq_operations mod_seq_ops = {
    .start = mod_start,
    .stop = mod_stop,
    .next = mod_next,
    .show = mod_show,
};

int mod_open(struct inode *inode, struct file *filp)
{
    int ret;
    struct module *mod;
    struct list_head *modules;
    struct mod_res *mod_res;
    struct seq_file *m;

    if (!(modules = (struct list_head *)kallsyms_lookup_name("modules"))) {
        printk(KERN_ERR "kernel symbol modules not found\n");
        return -ENOENT;
    }
    if (list_empty(modules)) {
        printk(KERN_ERR "No module loaded in this system\n");
        return -ENOENT;
    }
    mod = list_entry(modules->next, struct module, list);

    mod_res = (struct mod_res *)kmalloc(sizeof(*mod_res), GFP_KERNEL);
    if (!mod_res) {
        printk(KERN_ERR "No memory\n");
        return -ENOMEM;
    }
    memset(mod_res, 0, sizeof(*mod_res));
    mod_res->head = modules;
    mod_res->mod = mod;

    ret = seq_open(filp, &mod_seq_ops);
    if (ret) {
        kfree(mod_res);
        return ret;
    }

    m = (struct seq_file *)filp->private_data;
    m->private = mod_res;

    return 0;
}

int mod_flush(struct file *filp, fl_owner_t id)
{
    struct mod_res *mod_res = (struct mod_res *)((struct seq_file *)filp->private_data)->private;
    if (mod_res) {
        kfree(mod_res);
    }
    struct inode *inode = filp->f_path.dentry->d_inode;
    return seq_release(inode, filp);
}

const struct file_operations mod_fops = {
    .open = mod_open,
    .read = seq_read,
    .flush = mod_flush,
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

