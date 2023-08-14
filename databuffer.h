#ifndef DATABUFFER_H_INCLUDED
#define DATABUFFER_H_INCLUDED

#include <stdbool.h>
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
u8           databuffer_get_byte(Data_buffer* buf, u64 pos);
bool         databuffer_set_byte(Data_buffer* buf, u64 pos, u8 byt);
void         databuffer_reset(Data_buffer* buf);
void         databuffer_free(Data_buffer* buf);

#endif // DATABUFFER_H_INCLUDED
