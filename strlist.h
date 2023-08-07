#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#include <stdlib.h>

typedef struct {
    wchar_t* str;
    int size;
} Wide_string;

typedef struct {
    int* locations;
    wchar_t* data;
    int wchars_allocated;
    int wchars_used;
    int item_count;
} Wide_string_list;

Wide_string_list* wstrlist_new(int insize);
void wstrlist_add(Wide_string_list* l, wchar_t* str, int sz); /* We expect str to end with a terminator (\0) */
void wstrlist_add_and_terminator(Wide_string_list* l, wchar_t* str, int sz); /* For str without terminator */
Wide_string wstrlist_get(Wide_string_list* l, int pos);
void wstrlist_free(Wide_string_list* l);
void wstrlist_debug_print(Wide_string_list* l);

#endif // LIST_H_INCLUDED
