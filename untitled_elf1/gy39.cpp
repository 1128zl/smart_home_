#include "gy39.h"

Gy39::Gy39()
{
    fd = init_serial(COM7,9600);
}

void Gy39::run()
{
    char cmd[3] = {0xA5,0x52,0xF7};
    char data[15];

    while (1) {
    write(fd,cmd,3);
    int ret = read(fd,data,15);
    printf("%d\n",ret);
    for(int i = 0;i < ret; i++)
    {
        printf("%x ",data[i]);
    }

    int temp = data[4]<<8 | data[5];
    int hum = data[10]<<8 | data[11];

    emit sendData(temp,hum);

    sleep(3);
    }
}

int Gy39::init_serial(const char *file, int baudrate)
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
