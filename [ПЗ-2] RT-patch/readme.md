# [ПЗ-2] RT-patch

### Первичный тест
Для начала проведём исследование параметров системы с установленным дистрибутивом ```Ubuntu 22.04.5 (kernel-5.15.0)```
```cyclictest --mlockall --priority=80 --interval=200 --distance=0```

Приведём результат ниже:

![image](https://github.com/user-attachments/assets/7ed48907-376d-4c9a-9b1f-84ffdbec0bf9)

### Сборка RTOS

Следуя следующим шагам:
+ Скачаем дистрибутив с нашей версией ядра и RT-patch для соответствующей версии
+ С помощью утилиты ```patch``` применим изменения к нашему скачанному ядру
+ Настроим конфигурации с включённым параметром ```PREEMPT-RT``` в ```make menuconfig```
+ Сделаем сборку ```make``` и установим модули ```make modules_install```
+ Установим наше ядро ```make install``` (команда переносит скомпилированное ядро в ```/boot```)
+ Изменим стандартные конфигурации GRUB2
  + Сделаем меню выбора ОС видимым: ```GRUB_TIMEOUT_STYLE=menu```
  + Изменим время показа: ```GRUB_TIMEOUT=20```
  + Отключим выключение ОС после первого обнаружения: ```GRUB_DISABLE_OS_PROBER=false```
 
Собранное ядро и модули:

![image](https://github.com/user-attachments/assets/0f2fde26-e462-449c-8f02-fa63dfc9ef81)

Параметры GRUB2:

![image](https://github.com/user-attachments/assets/f5ffa14f-4792-417f-a18b-6932d994819c)

Отображение операционных систем:

![image](https://github.com/user-attachments/assets/9426e481-6d26-4e82-ac35-6bb2e2b3a695)

### Вторичный тест

Проведём аналогичный тест для операционной системы реального времени:

![image](https://github.com/user-attachments/assets/0811d6c7-72d5-426f-9205-c8f2cba8ad83)

Как видно из результатов, разброс выполнения процессов во времени значительно уменьшился, а значит операционная система работает в штатном режиме.

### Источники
Обучающая статья: https://www.acontis.com/en/building-a-real-time-linux-kernel-in-ubuntu-preemptrt.html
