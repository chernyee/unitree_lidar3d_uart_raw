#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <stdint.h>

#define MAX_PORT_LENGTH (128)

typedef struct
{
#ifdef _WIN32
    void *handle;
#else
    int fd;
#endif
    int baudrate;
    char dev[MAX_PORT_LENGTH];
} serial_port_t;

int serial_open(serial_port_t *sp);
int serial_set_baudrate(serial_port_t *sp, int baudrate);
int serial_is_opened(serial_port_t *sp);
int serial_read(serial_port_t *sp, uint8_t *buf, int len);
int serial_write(serial_port_t *sp, const uint8_t *buf, int len);
void serial_close(serial_port_t *sp);

#endif
