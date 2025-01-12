
# [ПЗ-1] Вывод с периодом 

### Задание
1) Разработать модуль ядра с параметром
2) Запустить поток ядра, который с заданной частотой выводит сообщение заданное параметром
3) Вставить модуль в исходники ядра


### Особенности
1) Т. к. разработка осуществлялась с помощью WSL2, а значит использовалось не стандартное ядро, а предоставленное компание Microsoft (https://github.com/microsoft/WSL2-Linux-Kernel/tree/linux-msft-wsl-5.10.y).

2) Т. к. стандартный подход был ограничен пришлось пересобирать ядро.

3) Ввиду того что сборка модуля осуществлялась не для текущей версии Ubuntu, а для «устаревшей» (близжайшая ветка в репозитории WSL2-Linux-Kernel), подключение модуля ядра осуществляется через команду
insmod -force

### Команды
Команда запуска модуля, где параметр ```text``` – сообщение, ```freq_write``` – частота вывода:

```
root@DESKTOP-MG69BAQ:/lib/workspace/sleep_print# sudo insmod -f sleep_print.ko text="HelloDrivers" freq_write=300
```
Команда остановки модуля:

```
root@DESKTOP-MG69BAQ:/lib/workspace/sleep_print# sudo rmmod sleep_print.ko
```
Команда просмотра сообщений:
```
root@DESKTOP-MG69BAQ:/lib/workspace/sleep_print# sudo dmesg
```
### Скриншот
![image](https://github.com/user-attachments/assets/b0aa89aa-f1b8-4674-89dd-23eceb84bcce)
