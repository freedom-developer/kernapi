#include <linux/module.h>
#include <linux/init.h>

#include <linux/device.h>

MODULE_LICENSE("GPL");

struct bus_type my_bus = {
    .name = "wsb",
};

static void mydev_release(struct device *dev)
{
    printk(KERN_INFO "release my_dev device\n");
}

struct device my_dev = {
    .init_name = "wsb_dev",
    .bus = &my_bus,
    .release = mydev_release,
};

struct device_driver my_driver = {
    .name = "wsb_driver",
    .bus = &my_bus,
    .owner = THIS_MODULE
};

static int __init cdev_init(void)
{
    int ret;

    ret = bus_register(&my_bus);
    if (ret < 0) {
        printk(KERN_ERR "register my bus failed\n");
        return -1;
    }

    ret = device_register(&my_dev);
    if (ret < 0) {
        bus_unregister(&my_bus);
        printk(KERN_ERR "add my_dev failed\n");

        return -1;
    }

    ret = driver_register(&my_driver);
    if (ret < 0) {
        printk(KERN_ERR "register my_driver failed\n");
        device_unregister(&my_dev);
        bus_unregister(&my_bus);
        return -1;
    }


    
    printk(KERN_INFO "cdev module initialize OK!\n");
    return 0;
}

static void __exit cdev_exit(void)
{
    device_unregister(&my_dev);
    bus_unregister(&my_bus);
    driver_unregister(&my_driver);

    printk(KERN_WARNING"cdev module exit ok!\n");
}


module_init(cdev_init);
module_exit(cdev_exit);



