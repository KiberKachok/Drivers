#include <linux/init.h>         // Заголовок для функции инициализации модуля.
#include <linux/module.h>       // Заголовок для работы с модулями ядра.
#include <linux/version.h>      // Заголовок для проверки версии ядра.
#include <linux/wait.h>         // Для работы с механизмами ожидания.
#include <linux/kthread.h>      // Для работы с потоками ядра.
#include <linux/delay.h>        // Для функций задержки (например, msleep).
#include <linux/moduleparam.h>  // Для работы с параметрами модуля.

MODULE_LICENSE("GPL");          // Лицензия модуля: GNU Public License.

/* Параметры модуля. */
static char *text = "Hello"; // Строка, которая будет выводиться в журнал ядра.
module_param(text, charp, 0); // Параметр модуля, который можно передать при загрузке.
static int freq_write = 1000;    // Частота задержки в миллисекундах.
module_param(freq_write, int, 0); // Параметр модуля, задающий задержку.

/* Переменная для хранения указателя на поток ядра. */
struct task_struct *ts;

/* Функция, выполняемая в потоке ядра. */
int thread(void *data) {
  while (1) { 
      printk(text);       // Вывод строки в журнал ядра.
      msleep(freq_write);       // Задержка на заданное время.
      if (kthread_should_stop()) break; // Проверка, нужно ли завершить поток.
  }
  return 0; // Возвращаемое значение потока.
}

/* Функция инициализации модуля. */
int init_module(void) {
  printk(KERN_INFO "init_module() called\n"); // Сообщение о загрузке модуля через printk.
  ts = kthread_run(thread, NULL, "freq_print kthread"); // Создание и запуск потока.
  return 0; // Успешная инициализация.
}

/* Функция очистки модуля. */
void cleanup_module(void) {
  printk(KERN_INFO "cleanup_module() called from thread\n"); // Сообщение об очистке модуля.
  kthread_stop(ts); // Завершение работы потока.
}