#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QTime>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "gy39.h"

namespace Ui {
class Widget;
}

#define NBUFFS 5

struct Frame
{
    unsigned char *addr;	//保存映射得到帧缓冲区的首地址
    int len;	//帧缓冲的大小
    int width;	//宽
    int height; //高
    int index;	//索引
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    bool curtainType = true;


        void YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst);
        void v4l2_init();
        int  v4l2_open_device(const char *devname);
        void v4l2_set_format(int width, int height);
        void v4l2_get_format(int index);
        void v4l2_request_buffers();
        void v4l2_start_capture();
        void v4l2_print_addrs();
        void v4l2_mmap_enqueue();
        void v4l2_enqueue(int index);
        int v4l2_dequeue();
        struct Frame v4l2_get_frame();

        int init_serial(const char *file, int baudrate);

public slots:
    void onNewConnect();
    void dataRead();
    void jsonReply(QNetworkReply *);
    void gy39Value(int temp, int hum);
    void curtainOn();
    void curtainOff();
  //  void progressChange(); void gy39ValueSlot(gy39Value);

    void timeslot();



private:
    Ui::Widget *ui;
    QTcpServer *server;
    QTcpSocket *client;
    QDir dir;
    int fd;

//    Gy39Thread *gy39;
//        FormaldehydeThread *pom;
//        MQThread *mq;
//        camera *c;

        QTimer *m_timer1;
        unsigned char yuyv[320*240*2];
        unsigned char rgb[320*240*3];
        int fd_video;
        struct Frame frames[NBUFFS];
};

#endif // WIDGET_H
