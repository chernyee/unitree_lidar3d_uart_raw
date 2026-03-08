#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdlib.h>

typedef struct
{
    uint8_t *buffer;
    size_t size;

    volatile size_t head;
    volatile size_t tail;

} ring_buffer_t;

int ring_init(ring_buffer_t *rb, size_t size);
void ring_free(ring_buffer_t *rb);

size_t ring_write(ring_buffer_t *rb, const uint8_t *data, size_t len);
size_t ring_available(ring_buffer_t *rb);

uint8_t* ring_peek(ring_buffer_t *rb);
void ring_consume(ring_buffer_t *rb, size_t len);

#endif
