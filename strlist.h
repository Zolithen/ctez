#ifndef STRLIST_H_INCLUDED
#define STRLIST_H_INCLUDED

#include "types.h"
#include <stdlib.h>

// TODO: Change instances of Wide_string_list to just List
typedef struct {
    wchar_t* str;
    u32 size;
} Wide_string;

typedef struct {
    u32* locations;
    wchar_t* data;
    u32 wchars_allocated;
    u32 wchars_used;
    u32  item_count;
} Wide_string_list;

void wstrlist_init(Wide_string_list* l, u32 insize);
Wide_string_list* wstrlist_new(u32 insize);
void wstrlist_add(Wide_string_list* l, const wchar_t* str, u32 sz); /* We expect str to end with a terminator (\0) */
void wstrlist_add_and_terminator(Wide_string_list* l, const wchar_t* str, u32 sz); /* For str without terminator */
Wide_string wstrlist_get(Wide_string_list* l, u32 pos);
void wstrlist_free(Wide_string_list* l);
void wstrlist_debug_print(Wide_string_list* l);

#endif // STRLIST_H_INCLUDED
