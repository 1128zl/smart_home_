#ifndef GY39_H
#define GY39_H

#include <QObject>
#include <QThread>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define COM2 "/dev/ttymxc1"
#define COM3 "/dev/ttymxc2"
#define COM7 "/dev/ttymxc6"




class Gy39 : public QThread
{
    Q_OBJECT
public:
    Gy39();

    virtual void run();

    int init_serial(const char *file, int baudrate);
    signals:void sendData(int,int);

private:
    int fd;
};

#endif // GY39_H
