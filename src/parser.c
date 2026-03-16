#include "parser.h"
#include "unitree_lidar_protocol.h"
// #include "unitree_lidar_utilities.h"

#include <stdio.h>
#include <string.h>

#define HEADER0 0x55
#define HEADER1 0xAA
#define HEADER2 0x05
#define HEADER3 0x0A

#define TAIL0 0x00
#define TAIL1 0xFF

#define MAX_PACKET_SIZE (64*1024)

typedef enum
{
    FSM_SEARCH_HEADER,
    FSM_READ_HEADER,
    FSM_READ_PACKET,
    FSM_VERIFY_TAIL
} parser_state_t;

static parser_state_t state;

static uint32_t expected_packet_size;

static parser_stats_t stats;

FILE* fd = NULL;

static uint32_t crc32(const uint8_t *buf, uint32_t len)
{
    uint8_t i;
    uint32_t crc = 0xFFFFFFFF;
    while (len--)
    {
        crc ^= *buf++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = (crc >> 1);
        }
    }
    return ~crc;
}

static void dump_hex(uint8_t *data, int len)
{
    for(int i=0;i<len;i++)
    {
        printf("%02X ",data[i]);
        if((i+1)%16==0) printf("\n");
    }
    printf("\n");
}

static void dump_hex_to_file(uint8_t *data, int len)
{
    for(int i = 0; i < len; i++)
    {
        fprintf(fd, "%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            fprintf(fd, "\n");
    }
    fprintf(fd, "\n");
}

void parser_init(void)
{
    memset(&stats,0,sizeof(stats));
    state = FSM_SEARCH_HEADER;
}


parser_stats_t* parser_get_stats(void)
{
    return &stats;
}

void reset_stats(void)
{
    memset(&stats, 0, sizeof(stats));
}

static int header_match(uint8_t *p)
{
    return p[0]==HEADER0 &&
           p[1]==HEADER1 &&
           p[2]==HEADER2 &&
           p[3]==HEADER3;
}


void parser_process(ring_buffer_t *rb)
{
    while(1)
    {
        size_t avail = ring_available(rb);

        if(avail == 0)
            return;

        uint8_t *ptr = ring_peek(rb);

        switch(state)
        {

            case FSM_SEARCH_HEADER:
            {
                if(avail < 4)
                    return;

                if(!header_match(ptr))
                {
                    ring_consume(rb,1);
                    stats.bytes_discarded++;
                    break;
                }

                state = FSM_READ_HEADER;
                break;
            }


            case FSM_READ_HEADER:
            {
                if(avail < sizeof(FrameHeader))
                    return;

                FrameHeader *hdr = (FrameHeader*)ptr;

                expected_packet_size = hdr->packet_size;

                stats.packets_total++;

                if(expected_packet_size < sizeof(FrameHeader) + sizeof(FrameTail) ||
                   expected_packet_size > MAX_PACKET_SIZE)
                {
                    stats.packets_size_error++;

                    ring_consume(rb,1);
                    state = FSM_SEARCH_HEADER;
                    break;
                }

                state = FSM_READ_PACKET;

                break;
            }


            case FSM_READ_PACKET:
            {
                if(avail < expected_packet_size)
                    return;

                state = FSM_VERIFY_TAIL;

                break;
            }


            case FSM_VERIFY_TAIL:
            {
                uint8_t *packet = ring_peek(rb);

                uint8_t *tail = packet + expected_packet_size - 2;

                if(tail[0] != TAIL0 || tail[1] != TAIL1)
                {
                    stats.packets_tail_error++;

                    ring_consume(rb,1);
                    state = FSM_SEARCH_HEADER;

                    break;
                }

                stats.packets_valid++;

                uint32_t crc = crc32(packet + 12, expected_packet_size - 24);
                if (crc != *(uint32_t *)(tail - 10)) {
                    stats.packets_crc_error++;
                }

                if (fd)
                {
                    dump_hex_to_file(packet, expected_packet_size);
                }
                else
                {
                    dump_hex(packet, expected_packet_size);
                }

                ring_consume(rb, expected_packet_size);

                state = FSM_SEARCH_HEADER;

                break;
            }

        }
    }
}
