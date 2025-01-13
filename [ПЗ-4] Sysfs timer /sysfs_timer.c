#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>

#define DRIVER_NAME "symbol_timer_driver"
#define DEVICE_NAME "symbol_timer"
#define CLASS_NAME "symbol_timer_class"

static int major_number;
static struct class *driver_class;
static struct device *driver_device;

static struct timer_list my_timer;
static int global_counter = 0;
static int timer_running = 0;
static unsigned long timer_interval_ms = 1000; // Интервал таймера в миллисекундах

// Таймер callback
static void timer_callback(struct timer_list *t) {
    global_counter++;
    printk(KERN_INFO DRIVER_NAME ": Timer triggered, global_counter = %d\n", global_counter);

    // Перезапуск таймера, если он активен
    if (timer_running) {
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(timer_interval_ms));
    }
}

// Функция для запуска таймера
static ssize_t start_timer(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    if (!timer_running) {
        timer_running = 1;
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(timer_interval_ms));
        printk(KERN_INFO DRIVER_NAME ": Timer started\n");
    }
    return count;
}

// Функция для остановки таймера
static ssize_t stop_timer(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    if (timer_running) {
        del_timer(&my_timer);
        timer_running = 0;
        printk(KERN_INFO DRIVER_NAME ": Timer stopped\n");
    }
    return count;
}

// Функция для чтения значения глобального счетчика
static ssize_t get_counter(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", global_counter);
}

// Функция для сброса значения глобального счетчика
static ssize_t reset_counter(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    global_counter = 0;
    printk(KERN_INFO DRIVER_NAME ": Counter reset\n");
    return count;
}

// Функция для установки интервала таймера
static ssize_t set_timer_interval(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    unsigned long interval;
    if (kstrtoul(buf, 10, &interval) == 0 && interval > 0) {
        timer_interval_ms = interval;
        printk(KERN_INFO DRIVER_NAME ": Timer interval set to %lu ms\n", timer_interval_ms);
    } else {
        printk(KERN_WARNING DRIVER_NAME ": Invalid timer interval\n");
    }
    return count;
}

// Определение атрибутов устройства
static DEVICE_ATTR(start, S_IWUSR, NULL, start_timer);
static DEVICE_ATTR(stop, S_IWUSR, NULL, stop_timer);
static DEVICE_ATTR(counter, S_IRUGO, get_counter, NULL);
static DEVICE_ATTR(reset, S_IWUSR, NULL, reset_counter);
static DEVICE_ATTR(interval, S_IWUSR, NULL, set_timer_interval);

// Инициализация модуля
static int __init symbol_timer_init(void) {
    int result;

    // Регистрация символьного устройства
    major_number = register_chrdev(0, DEVICE_NAME, NULL);
    if (major_number < 0) {
        printk(KERN_ALERT DRIVER_NAME ": Failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO DRIVER_NAME ": Registered with major number %d\n", major_number);

    // Создание класса устройства
    driver_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(driver_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT DRIVER_NAME ": Failed to register device class\n");
        return PTR_ERR(driver_class);
    }
    printk(KERN_INFO DRIVER_NAME ": Device class created\n");

    // Создание устройства
    driver_device = device_create(driver_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(driver_device)) {
        class_destroy(driver_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT DRIVER_NAME ": Failed to create the device\n");
        return PTR_ERR(driver_device);
    }
    printk(KERN_INFO DRIVER_NAME ": Device created\n");

    // Создание файлов атрибутов
    device_create_file(driver_device, &dev_attr_start);
    device_create_file(driver_device, &dev_attr_stop);
    device_create_file(driver_device, &dev_attr_counter);
    device_create_file(driver_device, &dev_attr_reset);
    device_create_file(driver_device, &dev_attr_interval);

    // Инициализация таймера
    timer_setup(&my_timer, timer_callback, 0);

    return 0;
}

// Очистка модуля
static void __exit symbol_timer_exit(void) {
    // Удаление таймера
    del_timer_sync(&my_timer);

    // Удаление файлов атрибутов
    device_remove_file(driver_device, &dev_attr_start);
    device_remove_file(driver_device, &dev_attr_stop);
    device_remove_file(driver_device, &dev_attr_counter);
    device_remove_file(driver_device, &dev_attr_reset);
    device_remove_file(driver_device, &dev_attr_interval);

    // Удаление устройства и класса
    device_destroy(driver_class, MKDEV(major_number, 0));
    class_destroy(driver_class);

    // Освобождение номера устройства
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO DRIVER_NAME ": Module unloaded\n");
}

module_init(symbol_timer_init);
module_exit(symbol_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Symbol Timer Driver");
MODULE_VERSION("1.0");
