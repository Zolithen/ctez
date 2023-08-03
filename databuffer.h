#ifndef DATABUFFER_H_INCLUDED
#define DATABUFFER_H_INCLUDED

#include <stdlib.h>
#include "types.h"

/* Stores arbitrary data and handles resizing automatically */
typedef struct {
    u8* data;
    u64 cursize;
    u64 maxsize;
} Data_buffer;

Data_buffer* databuffer_new(u64 insize);
void         databuffer_add_byte(Data_buffer* buf, u8 byt);
void         databuffer_add_bytes(Data_buffer* buf, u8* bytes, size_t amount);
void         databuffer_free(Data_buffer* buf);

#endif // DATABUFFER_H_INCLUDED
