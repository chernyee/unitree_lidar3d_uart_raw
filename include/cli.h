#ifndef CLI_H
#define CLI_H

#include "serial_port.h"
#include "ring_buffer.h"

void cli_init(ring_buffer_t *rb, const char *dev, int baudrate);
void cli_loop(void);

#endif
