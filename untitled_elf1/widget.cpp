#include "widget.h"
#include "ui_widget.h"

#include <qdebug.h>
#include <QString>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>




QString high;
QString low;
QString textday;
QString textnight;
QString high1;
QString low1;
QString textday1;
QString textnight1;
QString high2;
QString low2;
QString textday2;
QString textnight2;

#define COM2 "/dev/ttymxc1"
#define COM3 "/dev/ttymxc2"
#define COM7 "/dev/ttymxc6"

char On[7] = {0x9a,0x09,0x80,0x00,0x0a,0xdd,0x5e};
char Off[7] = {0x9a,0x09,0x80,0x00,0x0a,0xcc,0x4f};
char Stop[7] = {0x9a,0x09,0x80,0x00,0x0a,0xee,0x6d};

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    //using myNamespace::Gy39;
   Gy39* gy39 = new Gy39;
   gy39->start();

   fd = init_serial(COM2, 2400);
    connect(gy39,SIGNAL(sendData(int,int)),this,SLOT(gy39Value(int,int)));

    connect(ui->curtainOn_pushButton,SIGNAL(clicked(bool)),this,SLOT(curtainOn()));
    connect(ui->curtainOff_pushButton,SIGNAL(clicked(bool)),this,SLOT(curtainOff()));

//        QTimer* t = new QTimer(this);
//        t->start(200);//启动定时器，并定时0.2秒
//        connect(t,SIGNAL(timeout()),this,SLOT(progressChange()));



    server = new QTcpServer();
    server->listen(QHostAddress::AnyIPv4, 8888);
    connect(server,SIGNAL(newConnection()),this,SLOT(onNewConnect()));
    QNetworkAccessManager *net = new QNetworkAccessManager(this);
        //构造一个URL对象
        QUrl url = QUrl("http://api.seniverse.com/v3/weather/daily.json?key=SHLTb8T3h8IMjIZXu&location=changsha&language=zh-Hans&unit=c&start=0&days=");
        //向URL发起get请求  QNetworkRequest(url) 构造一个请求对象
        net->get(QNetworkRequest(url));

        /*
         * QNetworkAccessManager 发起请求后如果服务器响应了数据，自动发射一个信号
        void finished(QNetworkReply *reply)
        */
        connect(net,SIGNAL(finished(QNetworkReply *)),this,SLOT(jsonReply(QNetworkReply *)));




        v4l2_init();

           m_timer1 = new QTimer(this);
           m_timer1->start(50);
           connect(m_timer1, &QTimer::timeout, this, &Widget::timeslot);


}

void Widget::curtainOff()
{
    if(curtainType)
    {
        write(fd,On,7);
        //event curbain be up
        curtainType = false;

    }else
    {
        //stop
        write(fd,Stop,7);
        curtainType = true;
    }
}

void Widget::curtainOn()
{
    if(curtainType)
    {
        write(fd,Off,7);
        //event curbain be down
        curtainType = false;
    }else
    {
        write(fd,Stop,7);
        //stop
        curtainType = true;
    }
}



int Widget::init_serial(const char *file, int baudrate)
{
    int fd;

      fd = open(file, O_RDWR);
      if (fd == -1)
      {
          perror("open device error:");
          return -1;
      }

      struct termios myserial;
      //清空结构体
      memset(&myserial, 0, sizeof (myserial));
      //O_RDWR
      myserial.c_cflag |= (CLOCAL | CREAD);
      //设置控制模式状态，本地连接，接受使能
      //设置 数据位
      myserial.c_cflag &= ~CSIZE;   //清空数据位
      myserial.c_cflag &= ~CRTSCTS; //无硬件流控制
      myserial.c_cflag |= CS8;      //数据位:8

      myserial.c_cflag &= ~CSTOPB;//   //1位停止位
      myserial.c_cflag &= ~PARENB;  //不要校验

      switch (baudrate)
      {
          case 2400:
              cfsetospeed(&myserial, B2400);  //设置波特率
              cfsetispeed(&myserial, B2400);
              break;
          case 9600:
              cfsetospeed(&myserial, B9600);  //设置波特率
              cfsetispeed(&myserial, B9600);
              break;
          case 115200:
              cfsetospeed(&myserial, B115200);  //设置波特率
              cfsetispeed(&myserial, B115200);
              break;
          case 19200:
              cfsetospeed(&myserial, B19200);  //设置波特率
              cfsetispeed(&myserial, B19200);
              break;
      }

      /* 刷新输出队列,清除正接受的数据 */
      tcflush(fd, TCIFLUSH);

      /* 改变配置 */
      tcsetattr(fd, TCSANOW, &myserial);

      return fd;
}


void Widget::timeslot()
{
    struct Frame f ;
    //获取摄像头一帧数据
    f = v4l2_get_frame();

    //存储一张图片的像素点（RGB）
    unsigned char dst[f.width*f.height*3];

    //将yuv422格式转换成RGB格式
    YUV422toRGB888(f.width, f.height, f.addr,dst);

    QImage image(dst , 320 , 240 , QImage::Format_RGB888);


    QMatrix matrix;
 //   matrix.rotate(-90.0);//旋转-90度
    image = image.transformed(matrix,Qt::FastTransformation);

    QPixmap pixmap2=QPixmap::fromImage(image);
    ui->cameraLabel->setAutoFillBackground(true);
    pixmap2=pixmap2.scaled(ui->cameraLabel->size(), Qt::KeepAspectRatio);//自适应/等比例
    ui->cameraLabel->setStyleSheet("background: black;");  // 标签背景
    ui->cameraLabel->setAlignment(Qt::AlignCenter);  // 图片居中

    ui->cameraLabel->setPixmap(pixmap2);

    // 入队
    v4l2_enqueue(f.index);
}


void Widget::YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst)
{
    int line, column;
    unsigned char *py, *pu, *pv;
    unsigned char *tmp = dst;

    py = src;
    pu = src + 1;
    pv = src + 3;
    #define CLIP(x) ((x) >= 0xFF ? 0xFF : ((x) <= 0x00 ? 0x00 : (x)))
    for (line = 0; line < height; ++line)
    {
        for (column = 0; column < width; ++column)
        {
            *tmp++ = CLIP((int)*py + ((int)*pu - 128) + ((103 * ((int)*pu - 128)) >> 8));
            *tmp++ = CLIP((int)*py - (88 * ((int)*pv - 128) >> 8) - ((183 * ((int)*pu - 128)) >> 8));
            *tmp++ = CLIP((int)*py + ((int)*pv - 128) + ((198 * ((int)*pv - 128)) >> 8));
            // increase py every time
            py += 2;
            // increase pu,pv every second time
            if ((column & 1) == 1)
            {
                pu += 4;
                pv += 4;
            }
        }
    }
}

void Widget::v4l2_init()
{
    //打开摄像头
    v4l2_open_device("/dev/video2");

    //设置格式
    v4l2_set_format(320, 240);

    //申请缓冲区
    v4l2_request_buffers();

    //映射
    v4l2_mmap_enqueue();

    //打印 映射后的地址
    v4l2_print_addrs();

    //启动
    v4l2_start_capture();

    printf("video init success!!\n");
}

int  Widget::v4l2_open_device(const char *devname)
{
    fd_video = open(devname, O_RDWR);
    if (fd_video == -1)
    {
        perror("open video error");
        exit(1);
    }
    printf("open video ok!!\n");

    return fd_video;
}

void Widget::v4l2_set_format(int width, int height)
{
    //检查宽高是否合法
    if (!((width == 160 || width == 320 || width == 640) && (height == 120 || height == 240 || height == 480)))
    {
        printf("width or height invalid!!\n");
        exit(1);
    }

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //可选
    printf("fd = %d\n", fd_video);
    int res = ioctl(fd_video, VIDIOC_S_FMT, &format);
    if (res == -1)
    {
        perror("set format error");
        exit(1);
    }

    printf("set format OK!!\n");
}

void Widget::v4l2_get_format(int index)
{
    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int res = ioctl(fd_video, VIDIOC_G_FMT, &format);
    if (res == -1)
    {
        perror("get format error");
        exit(1);
    }
    frames[index].width = format.fmt.pix.width;
    frames[index].height = format.fmt.pix.height;
}

void Widget::v4l2_request_buffers()
{
    struct v4l2_requestbuffers reqbuffers;
    reqbuffers.count = NBUFFS;
    reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffers.memory = V4L2_MEMORY_MMAP;
    int res = ioctl(fd_video, VIDIOC_REQBUFS, &reqbuffers);
    if (res == -1)
    {
        perror("request buffer error");
        exit(1);
    }
    printf("request buffer success!!\n");
}

void Widget::v4l2_start_capture()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int res = ioctl(fd_video, VIDIOC_STREAMON, &type);
    if (res == -1)
    {
        perror("stream on error");
        exit(1);
    }
}

void Widget::v4l2_print_addrs()
{
    //测试应用空间的帧缓冲的地址和长度
    int i;
    for (i = 0; i < NBUFFS; ++i)
    {
        printf("addr: %p, len: %d\n", frames[i].addr, frames[i].len);
    }
}

void Widget::v4l2_mmap_enqueue()
{
    int i;
    int res;
    for (i = 0; i < NBUFFS; ++i)
    {
        struct v4l2_buffer buffer;
        buffer.index = i;
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //查询
        res = ioctl(fd_video, VIDIOC_QUERYBUF, &buffer);
        if (res == -1)
        {
            perror("query buffer error");
            exit(1);
        }
        //映射
        frames[i].index = i;
        frames[i].len = buffer.length;
        frames[i].addr = (unsigned char *)mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_video, buffer.m.offset);
        if (frames[i].addr == MAP_FAILED)
        {
            perror("mmap error");
            exit(1);
        }
        //入队
        v4l2_enqueue(i);
    }
}

void Widget::v4l2_enqueue(int index)
{
    struct v4l2_buffer buffer;
    buffer.index = index;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //查询
    int res = ioctl(fd_video, VIDIOC_QUERYBUF, &buffer);
    if (res == -1)
    {
        perror("query buffer error");
        exit(1);
    }
    //入队
    res = ioctl(fd_video, VIDIOC_QBUF, &buffer);
    if (res == -1)
    {
        perror("add queue error");
        exit(1);
    }
    // printf("%d buffer enqueue success!!\n", index);
}

int Widget::v4l2_dequeue()
{
    struct v4l2_buffer buffer;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int res = ioctl(fd_video, VIDIOC_DQBUF, &buffer);
    if (res == -1)
    {
        perror("dequeue error");
        exit(1);
    }
    // printf("dequeue addr: %x, len = %d, \n", frames[buffer.index].addr, frames[buffer.index].len);
    return buffer.index;
}

struct Frame Widget::v4l2_get_frame()
{
    int index = v4l2_dequeue();
    v4l2_get_format(index);
    return frames[index];
}



//void Widget::progressChange()
//{

//}


void Widget::gy39Value(int temp, int hum)
{
//    qDebug << "temp:" << temp;
//    qDebug << "hum:" << hum;
    ui->TemperatureValue_label->setText(QString::number(temp/100));
    ui->HummidityValue_label->setText(QString::number(hum/100));
}

void Widget::jsonReply(QNetworkReply *reply)
{
    //判断服务器响应的状态code是否是200
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//    qDebug() << code;

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error occurred:" << reply->errorString();
        return ;
    }

    QByteArray byteData = reply->readAll(); //获取服务器响应数据
//    qDebug() << byteData;
    /*
    Json数据的解析：
    1、构造一个QJsonDocument对象
    2、通过QJsonDocument对象 构造一个 QJsonObject对象
    3、解析QJsonObject对象
    */
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(byteData, &err);
    if (err.error != QJsonParseError::NoError)
    {
        qDebug() << "QJsonDocument::fromJson error";
        return ;
    }

    QJsonObject rootObj = doc.object();
    /*
    解析Json串的诀窍：一定要非常的细心
    Json串的组成由一个一个的键值对组成
    键所对应的值可能是个字符串，可能是个数字，可能是另外一个Json串
    还可能是个数组，并且数组中的每个元素还是一个Json串
    */
    //通过value(键)可以获取某个键所对应的值
    QJsonArray date = rootObj.value("results").toArray();


    //将data这个键对应的值转换成一个Json对象
    //forecast这个键对应的值是个数组
    QJsonObject weather = date[0].toObject();
    QJsonArray daily = weather.value("daily").toArray();

   // QJsonArray  weather = data.value("daily").toArray();
    //qDebug() << daily;
    QJsonObject content = daily[0].toObject();
   // QJsonObject content = daily[0].toObject();
    QJsonObject content1 =  daily[1].toObject();
    QJsonObject content2 =  daily[2].toObject();


    high = content.value("high").toString();
    low = content.value("low").toString();
    textday = content.value("text_day").toString();
    textnight = content.value("text_night").toString();
    high1 = content1.value("high").toString();
    low1 = content1.value("low").toString();
    textday1 = content1.value("text_day").toString();
    textnight1 = content1.value("text_night").toString();
    high2 = content2.value("high").toString();
    low2 = content2.value("low").toString();
    textday2 = content2.value("text_day").toString();
    textnight2 = content2.value("text_night").toString();

    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QString pathname = dir.currentPath();

    qDebug()<<pathname;
    QImage *img=new QImage; //新建一个image对象
    if(textday=="晴"){
    img->load(pathname + "/img/1.jpg");
    }else if(textday=="大雨"||textday=="中雨"||textday=="小雨"){
        img->load(pathname + "/img/3.jpg");
        }else {
        img->load(pathname + "/img/2.jpg");
        }
    ui->p_label->setPixmap(QPixmap::fromImage(*img)); //将图片放入label，使用setPixmap,注意指针*img

    QImage *img1=new QImage; //新建一个image对象
    if(textnight=="晴"){
    img1->load(pathname + "/img/1.jpg");
    }else if(textnight=="大雨"||textnight=="中雨"||textnight=="小雨"){
        img1->load(pathname + "/img/3.jpg");
        }else {
        img1->load(pathname + "/img/2.jpg");
        }
    ui->p_label_2->setPixmap(QPixmap::fromImage(*img1)); //将图片放入label，使用setPixmap,注意指针*img

    QImage *img2=new QImage; //新建一个image对象
    if(textday1=="晴"){
    img2->load(pathname + "/img/1.jpg");
    }else if(textday1=="大雨"||textday1=="中雨"||textday1=="小雨"){
        img2->load(pathname + "/img/3.jpg");
        }else {
        img2->load(pathname + "/img/2.jpg");
        }
    ui->p_label_3->setPixmap(QPixmap::fromImage(*img2)); //将图片放入label，使用setPixmap,注意指针*img

    QImage *img3=new QImage; //新建一个image对象
    if(textnight1=="晴"){
    img3->load(pathname + "/img/1.jpg");
    }else if(textnight1=="大雨"||textnight1=="中雨"||textnight1=="小雨"){
        img3->load(pathname + "/img/3.jpg");
        }else {
        img3->load(pathname + "/img/2.jpg");
        }
    ui->p_label_4->setPixmap(QPixmap::fromImage(*img3)); //将图片放入label，使用setPixmap,注意指针*img

    QImage *img4=new QImage; //新建一个image对象
    if(textday2=="晴"){
    img4->load(pathname + "/img/1.jpg");
    }else if(textday2=="大雨"||textday2=="中雨"||textday2=="小雨"){
        img4->load(pathname + "/img/3.jpg");
        }else {
        img4->load(pathname + "/img/2.jpg");
        }
    ui->p_label_5->setPixmap(QPixmap::fromImage(*img4)); //将图片放入label，使用setPixmap,注意指针*img

    QImage *img5=new QImage; //新建一个image对象
    if(textnight2=="晴"){
    img5->load(pathname + "/img/1.jpg");
    }else if(textnight2=="大雨"||textnight2=="中雨"||textnight2=="小雨"){
        img5->load(pathname + "/img/3.jpg");
        }else {
        img5->load(pathname + "/img/2.jpg");
        }
    ui->p_label_6->setPixmap(QPixmap::fromImage(*img5)); //将图片放入label，使用setPixmap,注意指针*img

    //qDebug() << textday2;
    //qDebug() << textday2;

    ui->wTemperature_Label1->setText(high + "℃" + "~" + low  + "℃");
    ui->wTemperature_Label2->setText(high1 + "℃" + "~" + low1  + "℃");
    ui->wTemperature_Label3->setText(high2 + "℃" + "~" + low2  + "℃");
    ui->wType_Label1->setText(textday + "~" + textnight);
    ui->wType_Label2->setText(textday1 + "~" + textnight1);
    ui->wType_Label3->setText(textday2 + "~" + textnight2);
}

void Widget::onNewConnect()
{
    client = server->nextPendingConnection();
    qDebug()<<client->peerAddress().toString()<<QString::number(client->peerPort());
    connect(client,SIGNAL(readyRead()),this,SLOT(dataRead()));
}

void Widget::dataRead()
{
    QByteArray data = client->readAll();
    qDebug() << data;
    qDebug() << "int";
}


Widget::~Widget()
{
    delete ui;
}
