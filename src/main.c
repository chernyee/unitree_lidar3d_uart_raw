#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "parser.h"
#include "serial_port.h"
#include "ring_buffer.h"
#include "cli.h"

#define RING_BUFFER_SIZE (8 * 1024 * 1024)

static void print_usage(const char *prog)
{
    printf("Usage:\n");
    printf("  %s <serial_port> <baudrate>\n\n", prog);

    printf("Examples:\n");
    printf("  Linux  : %s /dev/ttyUSB0 4000000\n", prog);
    printf("  Windows: %s COM3 4000000\n", prog);
}


int main(int argc, char *argv[])
{
    serial_port_t serial;
    ring_buffer_t ring;

    const char *port;
    int baud;

    printf("Unitree Lidar UART Tool\n");

    if(argc < 3)
    {
        print_usage(argv[0]);
        return -1;
    }

    port = argv[1];
    baud = atoi(argv[2]);

    if(baud <= 0)
    {
        printf("Invalid baudrate\n");
        return -1;
    }

    /* initialize ring buffer */
    if(ring_init(&ring, RING_BUFFER_SIZE) != 0)
    {
        printf("Failed to allocate ring buffer\n");
        return -1;
    }

    /* open serial port */
    if(serial_open(&serial, port, baud) != 0)
    {
        printf("Failed to open serial port %s\n", port);
        ring_free(&ring);
        return -1;
    }

    printf("Serial opened: %s @ %d baud\n", port, baud);
    printf("Ring buffer size: %d MB\n", RING_BUFFER_SIZE / (1024*1024));

    /* initialize CLI */
    cli_init(&serial, &ring);

    /* initialize parser FSM */
    parser_init();

    /* run CLI */
    cli_loop();

    /* cleanup */
    serial_close(&serial);
    ring_free(&ring);

    printf("\nProgram terminated\n");

    return 0;
}
