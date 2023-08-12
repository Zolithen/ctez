#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "command.h"

#include "types.h"
#include "misc.h"
#include "wstr.h"
#include "databuffer.h"
#include "strlist.h"

#include "buffer.h"

Wide_string_list* command_parse(wchar_t* com, int coml) {
    Wide_string_list* ret = wstrlist_new(25);

    //int argc = 0;

    int argstart = 0;
    int on_string = false;
    bool whitespace_ignore = false;
    for (int i = 0; i < coml; i++) {
        wchar_t c = com[i];

        if ((c == '"') || ( (i == coml - 1) && (on_string) )) {
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

/*
open <file>
bind <window number> <buffer id>
*/
Command_response command_execute(Wide_string_list* com) {
    //wstrlist_debug_print(com);
    Command_response resp = { 0 };
    if (com->item_count >= 1) {
        Wide_string command = wstrlist_get(com, 0);
        if (wstrcmp(command.str, L"open", command.size, 5)) {

            if (comerror_args_expect_exactly(com, 1)) return resp;
            Wide_string file_name = wstrlist_get(com, 1);

            u8* resstr = wstrdgr(file_name.str, file_name.size);
            TBUFID id = tsFILE_open(resstr);
            if (TB_system_error != TBSE_OK) {
                comerror_file_not_found(&file_name);
                comerror_file_invalid();
                TB_system_error = TBSE_OK;
                free(resstr);
                return resp;
            }
            free(resstr);

            /*int chosen_buffer = TWIN1;
            ts_free_buffer(bwindows[chosen_buffer]->buf_id);
            bwindows[chosen_buffer]->buf_id = id;
            bwindow_buf_set_flags_on(bwindows[chosen_buffer], TB_UPDATED);*/
            fbw_add_entry(id, file_name.str, file_name.size);

        } else if (wstrcmp(command.str, L"bind", command.size, 5)) { // TODO: consider making an alternative version that only takes the window ID (more intuitive)

            if (comerror_args_expect_exactly(com, 2)) return resp;

            Wide_string _winid = wstrlist_get(com, 1);
            Wide_string _bufid = wstrlist_get(com, 2);

            if ((wstrisnum(_winid.str, _winid.size)) && (wstrisnum(_bufid.str, _bufid.size))) {
                u32 winid = wstrtonum(_winid.str, _winid.size);
                u32 bufid = wstrtonum(_bufid.str, _bufid.size);

                if ( (winid > 4) || ((bufid < 3) || (bufid >= TB_system.current_alloc )) ) {
                    bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGRANGE);
                    return resp;
                }

                bwindows[TWIN(winid)]->buf_id = bufid;
                bwindow_buf_set_flags_on(bwindows[TWIN(winid)], TB_UPDATED);
            } else {
                bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGTYPE);
                return resp;
            }

            /*
            if (comerror_args_expect_exactly(com, 2)) return resp;
            if (comerror_args_type_expect_exactly(com, COMARGTYPE_INT, COMARGTYPE_INT)) return resp;
            if (comerror_args_range_expect_exactly(com, 0, 5, 2, TB_system.current_alloc + 1)) return resp;

            Wide_string _winid = wstrlist_get(com, 1);
            Wide_string _bufid = wstrlist_get(com, 2);
            u32 winid = wstrtonum(_winid.str, _winid.size);
            u32 bufid = wstrtonum(_bufid.str, _bufid.size);
            bwindows[TWIN(winid)]->buf_id = bufid;
            bwindow_buf_set_flags_on(bwindows[TWIN(winid)], TB_UPDATED);
            */

        } else {
            bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID);
            return resp;
        }
    } else {
        //resp.msg.str = L"No command given";
    }


    return resp;
}

bool comerror_args_expect_exactly(Wide_string_list* l, int numargs) {
    if (l->item_count - 1 != (u32)numargs) {
        Wide_string msg = { 0 };
        int chars_to_alloc = _scwprintf(STR_COMMSG_NEED_EXACTARGS.str, numargs) + 1; // _scwprintf doesn't count the terminating char
        msg.size = chars_to_alloc;
        msg.str = emalloc(chars_to_alloc * sizeof(wchar_t));
        _snwprintf_s(msg.str, chars_to_alloc, chars_to_alloc - 1, STR_COMMSG_NEED_EXACTARGS.str, numargs);
        bwindow_buf_insert_text(bwindows[TWINCOM], msg);
        free(msg.str);
        return true;
    }
    return false;
}

void comerror_file_not_found(Wide_string* file_name) {
    if (TB_system_error == TBSE_FILE_NOT_FOUND) {
        /* File not found <file_name>\n */
        Wide_string msg = { 0 };
        int s1_size = 0;
        wchar_t* s1 = wstrcat(L"File not found ", file_name->str, 16, file_name->size, &s1_size);
        msg.str = wstrcat(s1, L"\n", s1_size, 2, NULL);
        msg.size = s1_size + 1;

        bwindow_buf_insert_text(bwindows[TWINCOM], msg);
        free(msg.str);
        free(s1);
    }
}

void comerror_file_invalid() {
    if (TB_system_error == TBSE_INVALID_FILE) {
        Wide_string msg = { 0 };
        msg.str = L"File invalid\n";
        msg.size = 14;
        bwindow_buf_insert_text(bwindows[TWINCOM], msg);
    }
}
