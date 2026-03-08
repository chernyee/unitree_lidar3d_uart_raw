#ifndef CLI_H
#define CLI_H

#include "serial_port.h"
#include "ring_buffer.h"

void cli_init(serial_port_t *sp, ring_buffer_t *rb);
void cli_loop(void);

#endif
