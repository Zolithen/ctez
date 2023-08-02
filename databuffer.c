#include "databuffer.h"
#include "misc.h"
#include <stdlib.h>
#include <string.h>

Data_buffer* databuffer_new(unsigned long insize) {
    Data_buffer* buf = emalloc(sizeof(Data_buffer));
    buf->maxsize = insize;
    buf->data = ecalloc(insize, sizeof(unsigned char));
    buf->cursize = 0;

    return buf;
}

void databuffer_add_byte(Data_buffer* buf, unsigned char byt) {
    if (buf->cursize == buf->maxsize) {
        buf->maxsize *= 2;
        buf->data = erealloc(buf->data, buf->maxsize);
    }
    buf->data[buf->cursize] = byt;
    buf->cursize++;
}

void databuffer_add_bytes(Data_buffer* buf, unsigned char* bytes, size_t amount) {
    if (buf->cursize + amount >= buf->maxsize) {
        buf->maxsize += amount;
        buf->data = erealloc(buf->data, buf->maxsize);
    }

    memcpy(buf->data + buf->cursize, bytes, amount);
    buf->cursize += amount;
}

void databuffer_free(Data_buffer* buf) {
    free(buf->data);
    free(buf);
}
