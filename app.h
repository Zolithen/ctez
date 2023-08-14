#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

#include <stdbool.h>

#include "types.h"
#include "list.h"
#include "strlist.h"
#include "buffer.h"

struct {
    struct {
        List ids;
        List binds;
        Wide_string_list file_names;
        Wide_string_list names;
        TBUFID fb;
    } fbw;
    Buffer_window** bwindows;
    int layout;
    int selwin;
    int last_selwin;
    bool is_running;
} App;

void print_twincom(Wide_string msg);
void printf_twincom(const wchar_t* formatstring, ...);

#endif // APP_H_INCLUDED
