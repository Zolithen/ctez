#ifndef DATABUFFER_H_INCLUDED
#define DATABUFFER_H_INCLUDED

#include <stdlib.h>

/* Stores arbitrary data and handles resizing automatically */
typedef struct {
    unsigned char* data;
    unsigned long  cursize;
    unsigned long  maxsize;
} Data_buffer;

Data_buffer* databuffer_new(unsigned long insize);
void         databuffer_add_byte(Data_buffer* buf, unsigned char byt);
void         databuffer_add_bytes(Data_buffer* buf, unsigned char* bytes, size_t amount);
void         databuffer_free(Data_buffer* buf);

#endif // DATABUFFER_H_INCLUDED
