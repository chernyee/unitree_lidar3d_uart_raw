#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <stdint.h>

typedef struct
{
#ifdef _WIN32
    void *handle;
#else
    int fd;
#endif
    int baudrate;
} serial_port_t;

int serial_open(serial_port_t *sp, const char *dev, int baud);
int serial_set_baud(serial_port_t *sp, int baud);
int serial_read(serial_port_t *sp, uint8_t *buf, int len);
int serial_write(serial_port_t *sp, const uint8_t *buf, int len);
void serial_close(serial_port_t *sp);

#endif
