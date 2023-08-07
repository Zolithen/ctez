#include "databuffer.h"
#include "misc.h"
#include <stdlib.h>
#include <string.h>

Data_buffer* databuffer_new(u64 insize) {
    Data_buffer* buf = emalloc(sizeof(Data_buffer));
    buf->maxsize = insize;
    buf->data = ecalloc(insize, sizeof(u8));
    buf->cursize = 0;

    return buf;
}

void databuffer_add_byte(Data_buffer* buf, u8 byt) {
    if (buf->cursize == buf->maxsize) {
        buf->maxsize *= 2;
        buf->data = erealloc(buf->data, buf->maxsize);
    }
    buf->data[buf->cursize] = byt;
    buf->cursize++;
}

void databuffer_add_bytes(Data_buffer* buf, u8* bytes, size_t amount) {
    if (buf->cursize + amount >= buf->maxsize) {
        buf->maxsize += amount;
        buf->data = erealloc(buf->data, buf->maxsize);
    }

    memcpy(buf->data + buf->cursize, bytes, amount);
    buf->cursize += amount;
}

void databuffer_reset(Data_buffer* buf) {
    memset(buf->data, 0, buf->cursize);
}

void databuffer_free(Data_buffer* buf) {
    free(buf->data);
    free(buf);
}
