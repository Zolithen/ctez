#include "strlist.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>

Wide_string_list* wstrlist_new(int insize) {
    Wide_string_list* l = ecalloc(1, sizeof(Wide_string_list));
    l->item_count = 0;
    l->wchars_allocated = insize;
    l->wchars_used = 0;
    l->locations = ecalloc(1, sizeof(int));
    l->data = ecalloc(insize, sizeof(wchar_t));
    return l;
}
void wstrlist_add(Wide_string_list* l, wchar_t* str, int sz) {
    l->item_count++;
    l->locations = erealloc(l->locations, sizeof(int)*(l->item_count + 1));
    if (l->wchars_used + sz >= l->wchars_allocated) {
        l->wchars_allocated += sz + 2; // TODO: Look if we need to allocate more to use less reallocs
        l->data = erealloc(l->data, l->wchars_allocated*sizeof(wchar_t));
    }

    l->locations[l->item_count-1] = l->wchars_used;
    memcpy(l->data + l->wchars_used, str, sz * sizeof(wchar_t));
    l->wchars_used += sz;
    l->locations[l->item_count] = l->wchars_used;
}

void wstrlist_add_and_terminator(Wide_string_list* l, wchar_t* str, int sz) {
    l->item_count++;
    l->locations = erealloc(l->locations, sizeof(int)*(l->item_count + 1));
    if (l->wchars_used + sz + 1 >= l->wchars_allocated) {
        l->wchars_allocated += sz + 3;
        l->data = erealloc(l->data, l->wchars_allocated*sizeof(wchar_t));
    }

    l->locations[l->item_count-1] = l->wchars_used;
    memcpy(l->data + l->wchars_used, str, (sz + 1) * sizeof(wchar_t));
    *(l->data + l->wchars_used + sz) = '\0'; // Super sketchy
    l->wchars_used += sz + 1;
    l->locations[l->item_count] = l->wchars_used;
}

Wide_string wstrlist_get(Wide_string_list* l, int pos) {
    Wide_string ret = { 0 };
    if (pos > l->item_count) return ret;
    if (pos == 0) ret.size = l->locations[pos+1];
    else ret.size = l->locations[pos+1] - l->locations[pos];
    ret.str = l->data + l->locations[pos];
    return ret;
}

void wstrlist_free(Wide_string_list* l) {
    free(l->data);
    free(l->locations);
    free(l);
}

void wstrlist_debug_print(Wide_string_list* l) {
    printf("WSL DEBUG INFO START\n");
    for (int i = 0; i < l->item_count; i++) {
        printf("For index %d, location %d, wstring '%S'\n", i, l->locations[i], l->data + l->locations[i] );
    }

    printf("alloc, used, count: %d,%d,%d\n", l->wchars_allocated, l->wchars_used, l->item_count);
    printf("WSL DEBUG INFO END\n");
}
