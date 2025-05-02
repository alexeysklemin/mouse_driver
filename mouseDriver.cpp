#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/input.h>
#include <sys/ioctl.h>

class ComPortMouse {
private:
    int fd;
    struct termios tty;
    const char* device;

public:
    ComPortMouse(const char* dev) : device(dev), fd(-1) {}

    ~ComPortMouse() {
        if (fd != -1) close(fd);
    }

    bool initialize() {
        // Открываем последовательный порт
        fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
        if (fd == -1) {
            perror("Unable to open serial port");
            return false;
        }

        // Настраиваем параметры порта
        memset(&tty, 0, sizeof(tty));
        if (tcgetattr(fd, &tty) != 0) {
            perror("Error getting terminal attributes");
            return false;
        }

        // Устанавливаем скорость 1200 бод (стандартная для старых мышей)
        cfsetospeed(&tty, B1200);
        cfsetispeed(&tty, B1200);

        // 7 бит данных, четность, 1 стоп-бит (типичный формат для мышей)
        tty.c_cflag &= ~PARENB; // No parity
        tty.c_cflag &= ~CSTOPB; // 1 stop bit
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS7;     // 7 data bits

        tty.c_cflag |= (CLOCAL | CREAD); // Включаем чтение и игнорируем управляющие линии
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Отключаем программное управление потоком
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Неканонический режим
        tty.c_oflag &= ~OPOST; // Необработанный вывод

        // Устанавливаем таймауты: 100 мс между символами, 0 секунд между пакетами
        tty.c_cc[VMIN] = 3;    // Ожидаем 3 байта (типичный пакет мыши)
        tty.c_cc[VTIME] = 10;  // 1 секунда таймаута

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            perror("Error setting terminal attributes");
            return false;
        }

        return true;
    }

    void readMouseData() {
        unsigned char packet[3];
        int bytes_read;

        while (true) {
            bytes_read = read(fd, packet, sizeof(packet));
            if (bytes_read == sizeof(packet)) {
                // Разбираем пакет мыши (формат зависит от конкретной мыши)
                int left_button = (packet[0] & 0x20) ? 1 : 0;
                int right_button = (packet[0] & 0x10) ? 1 : 0;
                
                // Движение по X и Y (знаковые значения)
                int dx = (packet[0] & 0x03) << 6 | (packet[1] & 0x3F);
                int dy = (packet[0] & 0x0C) << 4 | (packet[2] & 0x3F);
                
                // Преобразование в знаковые значения
                if (dx > 127) dx -= 256;
                if (dy > 127) dy -= 256;
                
                printf("Buttons: L=%d R=%d | Movement: dx=%d dy=%d\n", 
                       left_button, right_button, dx, dy);
            } else if (bytes_read == -1) {
                perror("Error reading from serial port");
                break;
            }
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <serial_device>\n", argv[0]);
        printf("Example: %s /dev/ttyS0\n", argv[0]);
        return 1;
    }

    ComPortMouse mouse(argv[1]);
    if (!mouse.initialize()) {
        return 1;
    }

    printf("Mouse driver initialized. Reading data...\n");
    mouse.readMouseData();

    return 0;
}