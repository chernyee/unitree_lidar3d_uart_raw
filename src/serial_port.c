#include "serial_port.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>

static int configure_port(serial_port_t *sp, int baud)
{
    DCB dcb;

    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);

    if(!GetCommState(sp->handle, &dcb))
        return -1;

    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY;

    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if(!SetCommState(sp->handle, &dcb))
        return -1;

    COMMTIMEOUTS timeout;
    memset(&timeout,0,sizeof(timeout));

    timeout.ReadIntervalTimeout = 1;
    timeout.ReadTotalTimeoutConstant = 1;
    timeout.ReadTotalTimeoutMultiplier = 1;

    SetCommTimeouts(sp->handle,&timeout);

    return 0;
}

int serial_open(serial_port_t *sp)
{
    char path[64];

    snprintf(path,sizeof(path),"\\\\.\\%s",sp->dev);

    sp->handle = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if(sp->handle == INVALID_HANDLE_VALUE)
    {
        printf("Failed opening serial port\n");
        sp->handle = NULL;
        return -1;
    }

    SetupComm(sp->handle, 1<<20, 1<<20);

    if(configure_port(sp, sp->baudrate) != 0) {
        CloseHandle(sp->handle);
        sp->handle = NULL;
        return -1;
    }

    return 0;
}

int serial_is_opened(serial_port_t *sp)
{
    return sp->handle ? 1 : 0;
}

int serial_set_baudrate(serial_port_t *sp, int baudrate)
{
    if(configure_port(sp, baudrate) != 0)
        return -1;

    sp->baudrate = baudrate;

    return 0;
}

int serial_read(serial_port_t *sp, uint8_t *buf, int len)
{
    DWORD read_bytes = 0;

    if(!ReadFile(sp->handle, buf, len, &read_bytes, NULL))
        return -1;

    return (int)read_bytes;
}

int serial_write(serial_port_t *sp, const uint8_t *buf, int len)
{
    DWORD written = 0;

    if(!WriteFile(sp->handle, buf, len, &written, NULL))
        return -1;

    return (int)written;
}

void serial_close(serial_port_t *sp)
{
    if(sp->handle)
        CloseHandle(sp->handle);
    sp->handle = NULL;
}

#else

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

static int set_baudrate(struct termios *tty, int baud)
{
    speed_t speed;

    switch(baud)
    {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 921600: speed = B921600; break;

#ifdef B2000000
        case 2000000: speed = B2000000; break;
#endif
#ifdef B3000000
        case 3000000: speed = B3000000; break;
#endif
#ifdef B4000000
        case 4000000: speed = B4000000; break;
#endif

        default:
            return -1;
    }

    cfsetospeed(tty, speed);
    cfsetispeed(tty, speed);

    return 0;
}

int serial_is_opened(serial_port_t *sp)
{
    return sp->fd ? 1 : 0;
}

int serial_open(serial_port_t *sp)
{
    sp->fd = open(sp->dev, O_RDWR | O_NOCTTY);

    if(sp->fd < 0)
    {
        perror("open serial");
        sp->fd = 0;
        return -1;
    }

    int flags = fcntl(sp->fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(sp->fd, F_SETFL, flags | O_NONBLOCK);
    }

    struct termios tty;

    memset(&tty,0,sizeof(tty));

    if(tcgetattr(sp->fd, &tty) != 0)
    {
        perror("tcgetattr");
        close(sp->fd);
        sp->fd = 0;
        return -1;
    }

    set_baudrate(&tty, sp->baudrate);

    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;

    tcsetattr(sp->fd, TCSANOW, &tty);

    return 0;
}

int serial_set_baudrate(serial_port_t *sp, int baudrate)
{
    struct termios tty;

    if(tcgetattr(sp->fd,&tty) != 0)
        return -1;

    if(set_baudrate(&tty,baudrate) != 0)
        return -1;

    tcsetattr(sp->fd,TCSANOW,&tty);

    sp->baudrate = baudrate;

    return 0;
}

int serial_read(serial_port_t *sp, uint8_t *buf, int len)
{
    int n = read(sp->fd, buf, len);

    if(n < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }

    return n;
}

int serial_write(serial_port_t *sp, const uint8_t *buf, int len)
{
    return write(sp->fd, buf, len);
}

void serial_close(serial_port_t *sp)
{
    if(sp->fd > 0)
        close(sp->fd);
    sp->fd = 0;
}

#endif
