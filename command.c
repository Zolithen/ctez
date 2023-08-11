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
    Wide_string_list* ret = wstrlist_new(25);

    int argc = 0;

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
bound <window number> <buffer id>
*/
Command_response command_execute(Wide_string_list* com) {
    //wstrlist_debug_print(com);
    Command_response resp = { 0 };
    if (com->item_count >= 1) {
        Wide_string command = wstrlist_get(com, 0);
        if (wstrcmp(command.str, L"open", command.size, 5)) {


            Wide_string file_name = wstrlist_get(com, 1);
            if (file_name.str == NULL) {
                resp.resp = COMRESP_NEEDARGS;
                return resp;
            }

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



        } else if (wstrcmp(command.str, L"bound", command.size, 6)) { // TODO: consider making an alternative version that only takes the window ID (more intuitive)
            Wide_string _winid = wstrlist_get(com, 1);
            Wide_string _bufid = wstrlist_get(com, 2);

            if ((_winid.str == NULL) || (_bufid.str == NULL)) {
                bwindow_buf_insert_text(bwindows[TWINCOM], COMMAND_MSG_NEEDARGS);
                return resp;
            }

            if ((wstrisnum(_winid.str, _winid.size)) && (wstrisnum(_bufid.str, _bufid.size))) {
                int winid = wstrtonum(_winid.str, _winid.size);
                int bufid = wstrtonum(_bufid.str, _bufid.size);



            } else {
                bwindow_buf_insert_text(bwindows[TWINCOM], COMMAND_MSG_INVALID_ARGTYPE);
                return resp;
            }
        } else {
            resp.resp = COMRESP_INVALID;
        }
    } else {
        //resp.msg.str = L"No command given";
    }


    return resp;
}

void command_msg_setup_defaults() {
    COMMAND_MSG_NEEDARGS.str = L"Not enough arguments\n";
    COMMAND_MSG_NEEDARGS.size = 22;

    COMMAND_MSG_INVALID.str = L"Invalid command\n";
    COMMAND_MSG_INVALID.size = 17;

    COMMAND_MSG_INVALID_ARGTYPE.str = L"Invalid argument type\n";
    COMMAND_MSG_INVALID_ARGTYPE.size = 23;
}

void comerror_file_not_found(Wide_string* file_name) {
    if (TB_system_error == TBSE_FILE_NOT_FOUND) {
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
