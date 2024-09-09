#include <linux/module.h>
#include <linux/init.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");

int a_module(void);

int g_a = 100;

void pr_int(int a)
{
    printk(KERN_INFO "int var: %d\n", a);
}


void pr_module(struct module *m)
{
    int i;

    printk(KERN_INFO "mod name: %s\n", m->name);
    printk(KERN_INFO "refs: %d\n", module_refcount(m));
    printk(KERN_INFO "kernel symbols: %d\n", m->num_syms);
    for (i = 0; i < m->num_syms; i++) {
        printk(KERN_INFO "\tname: %s, value: %016lx\n", m->syms[i].name, m->syms[i].value);
    }

    printk(KERN_INFO "mem_layout: core_layout\n");
    printk(KERN_INFO "\tbase: %016lx, size: %d, text_size: %d, ro_size: %d, ro_after_init_size: %d\n",
            (unsigned long)m->core_layout.base, m->core_layout.size, m->core_layout.text_size, m->core_layout.ro_size, m->core_layout.ro_after_init_size);
}

void __module_address_test(void)
{
    struct module *ret;
    unsigned long addr = (unsigned long)&g_a;

    preempt_disable();
    ret = __module_address(addr);
    preempt_enable();

    if (ret) pr_module(ret);

    if (ret) {
        printk(KERN_INFO"ret->name: %s\n", ret->name);
        printk(KERN_INFO"ret->state: %d\n", ret->state);
        printk(KERN_INFO"ret->core_size: %d\n", ret->core_layout.size);
        printk(KERN_INFO"refs of %s is %d\n", ret->name, module_refcount(ret));
        printk(KERN_INFO "addr: %016lx\n", addr);
    } else {
        printk(KERN_ERR"__module_address return NULL!\n");
    }
}

void __module_text_address_test(void)
{
    unsigned long addr = (unsigned long)&a_module;
    struct module *ret;

    preempt_disable();
    ret = __module_text_address(addr);
    preempt_enable();
    printk(KERN_INFO "It's about function: \n");
    if (ret) {
        pr_module(ret);
        printk(KERN_INFO "ret->name: %s\n", ret->name);
        printk(KERN_INFO "ret->state: %d\n", ret->state);
    } else {
        printk(KERN_ERR "addr %016lx isn't a function\n", addr);
    }
}

void __print_symbol_test(unsigned long address)
{
    __print_symbol("symbol: %s\n", address);
}

void __symbol_get_test(void)
{
    int *pi = (int *)__symbol_get("rootfs_mount");
    if (!pi) {
        printk(KERN_ERR "no symbol g_a\n");
    }
    // void (*symf)(int) = (void (*)(int))__symbol_get("pr_int");
    // symf(*pi);
    // *pi += 100;
    // symf(*pi);
}

void find_module_test(const char *name)
{
    struct module *mod = find_module(name);
    if (!mod) {
        printk(KERN_ERR "find module %s failed\n", name);
        return;
    }
    pr_module(mod);
}

void find_symbol_test(const char *name)
{
    struct module *mod;
    const struct kernel_symbol *ks = find_symbol(name, &mod, NULL, true, true);
    if (!ks) {
        printk(KERN_ERR "kernel symbol %s not found\n", name);
        return;
    }

    printk(KERN_INFO "kernel symbol %s found, its module is : \n", name);
    if (mod)    pr_module(mod);
}

void *my_symbol_get(const char *name)
{
    const struct kernel_symbol *ks = find_symbol(name, NULL, NULL, true, true);
    if (!ks) {
        printk(KERN_ERR "kernel symbol %s not found\n", name);
        return NULL;
    }
    return (void *)ks->value;
}

void my_symbol_get_test(void)
{
    int *ga = (int *)my_symbol_get("g_a");
    void (*print)(int) = (void (*)(int))my_symbol_get("pr_int");
    if (!ga || !print) {
        return;
    }
    print(*ga);
    *ga += 100;
    print(*ga);
}


static int __init mymodule_init(void)
{
    my_symbol_get_test();

    return 0;
}

static void __exit mymodule_exit(void)
{
    printk(KERN_WARNING"module exit ok!\n");
}

int a_module(void)
{

    return 0;
}

EXPORT_SYMBOL(a_module);
EXPORT_SYMBOL(g_a);
EXPORT_SYMBOL(pr_int);

module_init(mymodule_init);
module_exit(mymodule_exit);

