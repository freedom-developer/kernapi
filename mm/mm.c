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
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/dcache.h>

MODULE_LICENSE("GPL");

const char *proc_name = "a.out";

#define FPBUFLEN 4096


struct fp {
    char buf[FPBUFLEN];
    size_t size;
    struct task_struct *p;
};

static int sp(char *buf, char *fmt, ...)
{
    int ret = 0;
    size_t len = FPBUFLEN;
    va_list ap;
    va_start(ap, fmt);
    if (len - strlen(buf) <= 0) {
        ret = -1;
    } else {
        vsnprintf(buf + strlen(buf), len - strlen(buf), fmt, ap);
    }
    va_end(ap);

    return ret;
}

void mm_show(struct fp *fp)
{
    struct task_struct *p = fp->p;
    char *buf = fp->buf;
    struct mm_struct *mm = p->mm;
    struct vm_area_struct *vma;
    int goon = 1;
    int idx = 0;
    char path_buf[512];

    if (!mm)
        return;
    for (vma = mm->mmap; vma && goon; vma = vma->vm_next) {
        if (vma->vm_next == mm->mmap)
            goon = 0;
        sp(buf, "vm_area %d: %016lx - %016lx, %s%s%s, %s\n", ++idx, vma->vm_start, vma->vm_end, 
            vma->vm_flags & VM_READ ? "r" : "-", 
            vma->vm_flags & VM_WRITE ? "w" : "-",
            vma->vm_flags & VM_EXEC ? "x" : "-",
            vma->vm_file ? d_path(&vma->vm_file->f_path, path_buf, 512) : "null");
    }
    sp(buf, "code: %016lx - %016lx\n", mm->start_code, mm->end_code);
    sp(buf, "data: %016lx - %016lx\n", mm->start_data, mm->end_data);
    sp(buf, "brk: %016lx - %016lx\n", mm->start_brk, mm->brk);
    sp(buf, "start stack: %016lx\n", mm->start_stack);
    sp(buf, "arg: %016lx - %016lx\n", mm->arg_start, mm->arg_end);
    sp(buf, "env: %016lx - %016lx\n", mm->env_start, mm->env_end);
    sp(buf, "total size: %ld\n", mm->task_size);

    fp->size = strlen(buf);
}

int mm_open(struct inode *inode, struct file *filp)
{
    struct task_struct *p;

    // 查找进程init_task
    for_each_process(p) {
        if (strcmp(p->comm, proc_name) == 0) {
            break;
        }
    }

    if (p == &init_task) {
        printk(KERN_ERR "process named %s not found\n", proc_name);
        return -ENOENT;
    }

    struct fp *fp = kmalloc(sizeof(*fp), GFP_KERNEL);
    if (!fp) {
        printk(KERN_ERR "No memory\n");
        return -ENOMEM;
    }
    // 填充数据到fp->buf中
    memset(fp, 0, sizeof(*fp));
    fp->p = p;
    
    mm_show(fp);

    filp->private_data = fp;

    printk(KERN_INFO "buf size: %ld\n", fp->size);

    return 0;
}

ssize_t mm_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    ssize_t copied;
    struct fp *fp = filp->private_data;
    ssize_t left = fp->size - *ppos;
    char *from = fp->buf;

    left = left <= 0 ? 0 : left;
    
    copied = len > left ? left : len;
    if (copy_to_user(buf, from + *ppos, copied)) {
        printk(KERN_ERR "copy to user error: %ld\n", copied);
        return -EFAULT;
    }
    (*ppos) += copied;
    len -= copied;

    return copied;
}

int mm_flush(struct file *filp, fl_owner_t id)
{
    if (filp->private_data) {
        kfree(filp->private_data);
    }

    return 0;
}

struct file_operations mm_fops = {
    .open = mm_open,
    .read = mm_read,
    .flush = mm_flush,
};

static int __init proc_mm_init(void)
{
    struct proc_dir_entry *mm;
    struct task_struct *p;

    // 添加文件/proc/mods
    if (!(mm = proc_create(proc_name, S_IRUGO|S_IWUSR, NULL, &mm_fops))) {
        printk(KERN_ERR "Failed to create /proc/%s file\n", proc_name);
        return -1;
    }

    return 0;
}

static void __exit proc_mm_exit(void)
{
    remove_proc_entry(proc_name, NULL);

    printk(KERN_WARNING"mm module exit ok!\n");
}


module_init(proc_mm_init);
module_exit(proc_mm_exit);



