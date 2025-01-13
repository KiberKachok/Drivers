#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/signal.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ktime.h>

// Определения для устройства и IOCTL команд
#define DEVICE_NAME "response_analyzer"
#define SET_PID_IOCTL _IOW('a', 'b', pid_t)
#define SIGNAL_RECEIVED_IOCTL _IO('a', 'd')
#define SIGNAL_CLOSE_IOCTL _IO('a', 'e')

// Константы для конфигурации
#define BINS 10
#define TIME_MODELING 60 * 5 // 5 минут

// Глобальные переменные
static pid_t user_pid = -1;
static int device_open_count = 0;
static int major;
static struct class *cls;

// Таймер для обработки сигналов
struct timer_list timer_;
unsigned long timer_duration = 1000;
ktime_t start_time;

// Переменные для хранения статистики времени
int time_value_count = 0;
s64 sum_time = 0;
s64 max_time = 0;
s64 min_time = 0;
s32 history_time[TIME_MODELING];
int hist[BINS];

/**
 * Обработчик таймера, вызывается каждые 1000 мс
 */
void timer_callback(struct timer_list *t) {
    if (user_pid != -1) {
        struct pid *pid_struct = find_get_pid(user_pid);

        if (pid_struct) {
            struct task_struct *task = pid_task(pid_struct, PIDTYPE_PID);

            if (task) {
                send_sig(SIGUSR1, task, 0);
                start_time = ktime_get_real();
            } else {
                pr_alert("Invalid task for PID: %d\n", user_pid);
            }
            put_pid(pid_struct);
        } else {
            pr_alert("Invalid PID: %d\n", user_pid);
        }
    }

    mod_timer(&timer_, jiffies + msecs_to_jiffies(timer_duration));
}

/**
 * Обработчик закрытия устройства
 */
void device_close_event(void) {
    pr_info("Average time: %lld ns\n", sum_time / time_value_count);
    pr_info("Max time: %lld ns\n", max_time);

    // Очистка гистограммы
    for (int i = 0; i < BINS; ++i) {
        hist[i] = 0;
    }

    // Построение гистограммы
    s64 step = (max_time - min_time) / BINS;
    for (int i = 0; i < time_value_count; ++i) {
        for (int k = 0; k < BINS; ++k) {
            if (history_time[i] >= min_time + (step * k) && history_time[i] < min_time + (step * (k + 1))) {
                ++hist[k];
            }
        }
        if (max_time == history_time[i]) {
            ++hist[BINS - 1];
        }
    }

    // Вывод результатов
    for (int k = 0; k < BINS; ++k) {
        pr_info("bins: %lld-%lld ns:\t %d\n", min_time + (step * k), min_time + (step * (k + 1)), hist[k]);
    }

    // Сброс счетчиков
    min_time = 0;
    max_time = 0;
    sum_time = 0;
    time_value_count = 0;
}

/**
 * Обработчик открытия устройства
 */
static int device_open(struct inode *inode, struct file *file) {
    if (device_open_count > 0) {
        return -EBUSY;
    }

    device_open_count++;
    pr_alert("Device open\n");
    return 0;
}

/**
 * Обработчик закрытия устройства
 */
static int device_release(struct inode *inode, struct file *file) {
    if (device_open_count <= 0) {
        return -EBUSY;
    }

    device_open_count--;
    user_pid = -1;
    device_close_event();
    pr_alert("Device close\n");
    return 0;
}

/**
 * Обработчик IOCTL запросов
 */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case SET_PID_IOCTL:
            if (copy_from_user(&user_pid, (pid_t *)arg, sizeof(pid_t))) {
                return -EFAULT;
            }
            break;
        case SIGNAL_RECEIVED_IOCTL: {
            ktime_t end_time = ktime_get_real(); // Получение текущего времени
            s64 elapsed_time = ktime_to_ns(ktime_sub(end_time, start_time)); // Вычисление разницы во времени
            sum_time += elapsed_time;
            history_time[time_value_count] = elapsed_time;

            if (elapsed_time > max_time) {
                max_time = elapsed_time;
            }

            if (time_value_count == 0 || elapsed_time < min_time) {
                min_time = elapsed_time;
            }

            time_value_count++;

            if (time_value_count == TIME_MODELING) {
                if (user_pid != -1) {
                    struct pid *pid_struct = find_get_pid(user_pid);
                    if (pid_struct) {
                        struct task_struct *task = pid_task(pid_struct, PIDTYPE_PID);
                        if (task) {
                            send_sig(SIGINT, task, 0);
                        } else {
                            pr_alert("Invalid task for PID: %d\n", user_pid);
                        }
                        put_pid(pid_struct);
                    } else {
                        pr_alert("Invalid PID: %d\n", user_pid);
                    }
                }
            }
            break;
        }
        default:
            return -EINVAL;
    }
    return 0;
}

// Структура операций с файлами
static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

/**
 * Инициализация драйвера
 */
static int __init my_driver_init(void) {
    struct device *dev;
    major = register_chrdev(0, DEVICE_NAME, &fops);

    if (major < 0) {
        pr_alert("register_chrdev() failed: %d\n", major);
        return -EINVAL;
    }

    pr_info("Major number: %d\n", major);

    cls = class_create(THIS_MODULE, DEVICE_NAME);
    if (!cls) {
        pr_alert("class_create() failed.\n");
        goto fail_class;
    }

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
            if (IS_ERR(dev)) {
            pr_alert("device_create() failed: %ld\n", PTR_ERR(dev));
            goto fail_class;
        }

        pr_info("/dev/%s created\n", DEVICE_NAME);

        timer_setup(&timer_, timer_callback, 0);
        mod_timer(&timer_, jiffies + msecs_to_jiffies(timer_duration));

        return 0;

    fail_class:
        unregister_chrdev(major, DEVICE_NAME);
        return -EINVAL;
    }

    /**
     * Завершение работы драйвера
     */
    static void __exit my_driver_exit(void) {
        device_destroy(cls, MKDEV(major, 0));
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        del_timer(&timer_);
    }

    module_init(my_driver_init);
    module_exit(my_driver_exit);
    MODULE_LICENSE("GPL");
    MODULE_DESCRIPTION("Простой драйвер для анализа временных интервалов");
    MODULE_AUTHOR("Expert");
