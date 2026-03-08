#ifndef PARSER_H
#define PARSER_H

#include "ring_buffer.h"
#include <stdint.h>

typedef struct
{
    uint64_t packets_total;
    uint64_t packets_valid;
    uint64_t packets_crc_error;
    uint64_t packets_tail_error;
    uint64_t packets_size_error;
    uint64_t bytes_processed;

} parser_stats_t;

void parser_init(void);
void parser_process(ring_buffer_t *rb);
const parser_stats_t* parser_get_stats(void);

#endif
