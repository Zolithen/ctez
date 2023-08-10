#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#include "types.h"

typedef struct {
    u8* data;
    u32 sz;
} Sized_pointer;

typedef struct {
    u32* locations;
    u8* data;
    u32 bytes_allocated;
    u32 bytes_used;
    u32 item_count;
} List;

void          list_init(List* l, u32 insize);
List*         list_new(u32 insize);
void          list_add(List* l, void* data, u32 sz);
Sized_pointer list_get(List* l, u32 pos);
void          list_free(List* l);
void          list_debug_print(List* l);

#endif // LIST_H_INCLUDED
