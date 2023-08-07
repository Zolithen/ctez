#include "command.h"
#include "databuffer.h"
#include "types.h"
#include "misc.h"
#include "strlist.h"
#include <string.h>
#include <stdbool.h>

Wide_string_list* command_parse(wchar_t* com, int coml) {
    Wide_string_list* ret = ecalloc(1, sizeof(Wide_string_list));

    int argc = 0;

    int argstart = 0;
    int on_string = false;
    bool whitespace_ignore = false;
    for (int i = 0; i < coml; i++) {
        wchar_t c = com[i];

        if (c == '"') {
            if (on_string) {
                wstrlist_add_and_terminator(ret, com + argstart, i - argstart);
            }
            argstart = i + 1;
            on_string = !on_string;
            whitespace_ignore = true;
            continue;
        }

        if ( ((c == ' ') || (i == coml - 1)) && (!on_string) ) {
            if (whitespace_ignore) {
                whitespace_ignore = false;
            } else {
                wstrlist_add_and_terminator(ret, com + argstart, i - argstart);
            }

            argstart = i + 1;
        }

    }

    return ret;

}

Command_response command_execute(Wide_string_list* com) {
    Command_response resp = { 0 };
    if (com->item_count >= 1) {
        Wide_string command = wstrlist_get(com, 0);
    } else {
        resp.resp = COMRESP_INVALID;
        //resp.msg.str = L"No command given";
        return resp;
    }
}
