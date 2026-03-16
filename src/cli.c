#include "cli.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#endif

extern FILE *fd;

static serial_port_t serial;
static ring_buffer_t *ring;

static volatile int running = 0;

#ifdef _WIN32
static HANDLE rx_thread_handle;
static HANDLE parser_thread_handle;
#else
static pthread_t rx_thread_handle;
static pthread_t parser_thread_handle;
#endif


void cli_init(ring_buffer_t *rb, const char *dev, int baudrate)
{
    serial.baudrate = baudrate;
    serial.fd = 0;
    strcpy(serial.dev, dev);
    ring = rb;
}


#ifdef _WIN32
DWORD WINAPI rx_thread(void *arg)
#else
void* rx_thread(void *arg)
#endif
{
    uint8_t buf[4096];

    parser_stats_t *stats = parser_get_stats();

    while(running)
    {
        int n = serial_read(&serial, buf, sizeof(buf));

        if(n > 0)
        {
            ring_write(ring, buf, n);
            stats->bytes_rx_total += n;
        }
    }

    return 0;
}


#ifdef _WIN32
DWORD WINAPI parser_thread(void *arg)
#else
void* parser_thread(void *arg)
#endif
{
    while(running)
    {
        parser_process(ring);
    }

    return 0;
}


static void wait_keypress()
{
#ifdef _WIN32

    while(!_kbhit())
        Sleep(10);

    _getch();

#else

    struct timeval tv;
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(0,&fds);

    select(1,&fds,NULL,NULL,NULL);

    getchar();

#endif
}

static void print_stats(void)
{
    parser_stats_t *stats = parser_get_stats();

    printf("\nParser statistics\n");
    printf("-----------------\n");
    printf("Packets total   : %llu\n", (unsigned long long)stats->packets_total);
    printf("Packets valid   : %llu\n", (unsigned long long)stats->packets_valid);
    printf("Size errors     : %llu\n", (unsigned long long)stats->packets_size_error);
    printf("Tail errors     : %llu\n", (unsigned long long)stats->packets_tail_error);
    printf("CRC errors      : %llu\n", (unsigned long long)stats->packets_crc_error);
    printf("Bytes discarded : %llu\n", (unsigned long long)stats->bytes_discarded);
    printf("Total bytes rx  : %llu\n", (unsigned long long)stats->bytes_rx_total);
}

static void start_dump(char *filename)
{
    printf("Starting dumping...\n");
    printf("Press any key to stop\n");

    /* reset stats */
    reset_stats();

    /* create dump file */
    fd = fopen(filename, "w");
    if (!fd)
    {
        printf("Failed to create dump file %s\n", filename);
        return;
    }

    /* open serial port */
    if(serial_open(&serial) != 0)
    {
        printf("Failed to open serial port %s @ %d baudrate\n", serial.dev, serial.baudrate);
        fclose(fd);
        fd = NULL;
        return;
    }

    running = 1;

#ifdef _WIN32

    rx_thread_handle = CreateThread(NULL,0,rx_thread,NULL,0,NULL);
    parser_thread_handle = CreateThread(NULL,0,parser_thread,NULL,0,NULL);

#else

    pthread_create(&rx_thread_handle,NULL,rx_thread,NULL);
    pthread_create(&parser_thread_handle,NULL,parser_thread,NULL);

#endif

    wait_keypress();

    printf("Dump Stoping...\n");

    running = 0;

    serial_close(&serial);

#ifdef _WIN32

    WaitForSingleObject(rx_thread_handle,INFINITE);
    WaitForSingleObject(parser_thread_handle,INFINITE);

#else

    pthread_join(rx_thread_handle,NULL);
    pthread_join(parser_thread_handle,NULL);

#endif

    printf("\nDump stopped\n");

    printf("\nDump file: %s\n", filename);

    /* close dump file */
    fclose(fd);
    fd = NULL;

    /* print rx stats */
    print_stats();
}

static void start_capture()
{
    printf("Starting capture...\n");
    printf("Press any key to stop\n");

    /* reset stats */
    reset_stats();

    /* open serial port */
    if(serial_open(&serial) != 0)
    {
        printf("Failed to open serial port %s @ %d baudrate\n", serial.dev, serial.baudrate);
        return;
    }

    running = 1;

#ifdef _WIN32

    rx_thread_handle = CreateThread(NULL,0,rx_thread,NULL,0,NULL);
    parser_thread_handle = CreateThread(NULL,0,parser_thread,NULL,0,NULL);

#else

    pthread_create(&rx_thread_handle,NULL,rx_thread,NULL);
    pthread_create(&parser_thread_handle,NULL,parser_thread,NULL);

#endif

    wait_keypress();

    printf("Capture stopping...\n");

    running = 0;

    serial_close(&serial);

#ifdef _WIN32

    WaitForSingleObject(rx_thread_handle,INFINITE);
    WaitForSingleObject(parser_thread_handle,INFINITE);

#else

    pthread_join(rx_thread_handle,NULL);
    pthread_join(parser_thread_handle,NULL);

#endif

    printf("\nCapture stopped\n");

    /* print rx stats */
    print_stats();
}


static void send_mode_command()
{
    uint8_t packet[64];

    memset(packet,0,sizeof(packet));

    packet[0] = 0x55;
    packet[1] = 0xAA;
    packet[2] = 0x05;
    packet[3] = 0x0A;

    uint32_t packet_type = 0x0A00; // example command type
    uint32_t packet_size = 24;

    memcpy(packet+4,&packet_type,4);
    memcpy(packet+8,&packet_size,4);

    packet[packet_size-2] = 0x00;
    packet[packet_size-1] = 0xFF;

    serial_write(&serial,packet,packet_size);

    printf("Mode command sent\n");
}

void cli_loop(void)
{
    char line[256];
    int i = 0;

    while(1)
    {
        printf("> ");
        fflush(stdout);

        if(!fgets(line,sizeof(line),stdin))
            break;

        if (strlen(line) <= 1)
            continue;

        i = 0;

        while (1)
        {
            if (line[i] == 10 || line[i] == 13) {
                line[i] = 0;
                break;
            }
            i++;
        }

        if(strncmp(line,"start",5) == 0)
        {
            start_capture();
        }

        else if(strncmp(line,"dump", 4) == 0)
        {
            start_dump(line+5);
        }

        else if(strncmp(line,"baudrate",8) == 0)
        {
            printf("Current baudrate: %d\n", serial.baudrate);
        }

        else if(strncmp(line,"set baudrate",12) == 0)
        {
            int baudrate = atoi(line+13);

            if(baudrate <= 0)
            {
                printf("Invalid baudrate\n");
                continue;
            }

            if (serial_is_opened(&serial))
            {
                if(serial_set_baudrate(&serial, baudrate) == 0)
                    printf("Baudrate set to %d\n", baudrate);
                else
                    printf("Failed setting baudrate\n");
            }
            else
            {
                serial.baudrate = baudrate;
            }
        }

        else if(strncmp(line,"set mode",8) == 0)
        {
            send_mode_command();
        }

        else if(strncmp(line,"exit",4) == 0)
        {
            break;
        }

        else
        {
            printf("Commands:\n");
            printf(" start\n");
            printf(" dump <filename>\n");
            printf(" baudrate\n");
            printf(" set baudrate <value>\n");
            printf(" set mode\n");
            printf(" exit\n");
        }
    }
}
