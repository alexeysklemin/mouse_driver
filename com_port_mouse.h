#ifndef COMPORTMOUSE_H
#define COMPORTMOUSE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <string>

class ComPortMouse {
public:
    // Конструктор с параметром - устройство последовательного порта
    explicit ComPortMouse(const char* dev);
    
    // Деструктор
    ~ComPortMouse();
    
    // Инициализация последовательного порта
    bool initialize();
    
    // Чтение данных с мыши (блокирующий вызов)
    void readMouseData();

private:
    int fd;                 // Файловый дескриптор порта
    struct termios tty;     // Структура с настройками порта
    const char* device;     // Путь к устройству порта
    
    // Запрещаем копирование и присваивание
    ComPortMouse(const ComPortMouse&) = delete;
    ComPortMouse& operator=(const ComPortMouse&) = delete;
};

#endif // COMPORTMOUSE_H