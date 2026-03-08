#include "ring_buffer.h"
#include <string.h>

int ring_init(ring_buffer_t *rb, size_t size)
{
    rb->buffer = malloc(size * 2);
    if(!rb->buffer) return -1;

    rb->size = size;
    rb->head = 0;
    rb->tail = 0;

    return 0;
}

void ring_free(ring_buffer_t *rb)
{
    free(rb->buffer);
}

size_t ring_available(ring_buffer_t *rb)
{
    return rb->head - rb->tail;
}

size_t ring_write(ring_buffer_t *rb, const uint8_t *data, size_t len)
{
    size_t pos = rb->head % rb->size;

    memcpy(rb->buffer + pos, data, len);
    memcpy(rb->buffer + pos + rb->size, data, len);

    rb->head += len;

    return len;
}

uint8_t* ring_peek(ring_buffer_t *rb)
{
    return rb->buffer + (rb->tail % rb->size);
}

void ring_consume(ring_buffer_t *rb, size_t len)
{
    rb->tail += len;
}
