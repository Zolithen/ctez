#include "command.h"
#include "databuffer.h"
#include "types.h"
#include "misc.h"
#include "strlist.h"
#include "buffer.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

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
        if (wstrcmp(command.str, L"open", command.size, 5)) {
            Wide_string file_name = wstrlist_get(com, 1);
            Wide_string buf = wstrlist_get(com, 2);
            TBUFID id = tsFILE_open(file_name.str); // TODO: turn this wstr to a normal str so that the function actually works
            if (TB_system_error != TBSE_OK) {
                if (TB_system_error == TBSE_FILE_NOT_FOUND) {
                    Wide_string msg = { 0 };
                    msg.str = L"File not found";
                    msg.size = 15;
                    bwindow_buf_insert_text(bwindows[TWINCOM], msg);
                }
                if (TB_system_error == TBSE_INVALID_FILE) {
                    Wide_string msg = { 0 };
                    msg.str = L"File invalid";
                    msg.size = 13;
                    bwindow_buf_insert_text(bwindows[TWINCOM], msg);
                }
                TB_system_error = TBSE_OK;
                return resp;
            }

            printf("how\n");
            ts_free_buffer(bwindows[TWIN1]->buf_id);
            bwindows[TWIN1]->buf_id = id;
            bwindow_buf_set_flags_on(bwindows[TWIN1], TB_UPDATED);

            //FILE* f = fopen(name, "r+");
        }
    } else {
        resp.resp = COMRESP_INVALID;
        //resp.msg.str = L"No command given";
    }


    return resp;
}
