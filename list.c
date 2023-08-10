#include "list.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void list_init(List* l, u32 insize) {
    l->item_count = 0;
    l->bytes_allocated = insize;
    l->bytes_used = 0;
    l->locations = ecalloc(1, sizeof(u32));
    l->data = ecalloc(insize, sizeof(u8));
}
List* list_new(u32 insize) {
    List* l = ecalloc(1, sizeof(List));
    list_init(l, insize);
    return l;
}

void list_add(List* l, void* data, u32 sz) {
    l->item_count++;
    l->locations = erealloc(l->locations, sizeof(u32)*(l->item_count + 1));
    if (l->bytes_used + sz >= l->bytes_allocated) {
        l->bytes_allocated += sz + 2; // TODO: Look if we need to allocate more to use less reallocs
        l->data = erealloc(l->data, l->bytes_allocated*sizeof(u8));
    }

    l->locations[l->item_count-1] = l->bytes_used;
    memcpy(l->data + l->bytes_used, data, sz * sizeof(u8));
    l->bytes_used += sz;
    l->locations[l->item_count] = l->bytes_used;
}

Sized_pointer list_get(List* l, u32 pos) {
    Sized_pointer ret = { 0 };
    if (pos >= l->item_count) return ret;
    if (pos == 0) ret.sz = l->locations[pos+1];
    else ret.sz = l->locations[pos+1] - l->locations[pos];
    ret.data = l->data + l->locations[pos];
    return ret;
}

/* This function may not free everything that is inside the list. If we have an struct inside the array that has
   a pointer to allocated memory, it's not freed. */
void list_free(List* l) {
    free(l->data);
    free(l->locations);
    free(l);
}
void list_debug_print(List* l) {
    printf("SPL DEBUG INFO START\n");
    for (u32 i = 0; i < l->item_count; i++) {
        printf("Item %d, from %d to %d\n", i, l->locations[i+1], l->locations[i]);
        //printf("For index %d, location %d, data '%s'\n", i, l->locations[i], l->data + l->locations[i] );
        for (u32 j = l->locations[i]; j < l->locations[i+1]; j++) {
            printf("%d\n", l->data[j]);
        }
    }

    printf("alloc, used, count: %d,%d,%d\n", l->bytes_allocated, l->bytes_used, l->item_count);
    printf("SPL DEBUG INFO END\n");
}
