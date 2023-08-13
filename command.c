#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

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
save <buffer id>
new

getpath <buffer id>
saveall
filebind <file path> <buffer id>

setprojectdir <dir path> Still not sure how to handle this
cd <path>
*/
void command_execute(Wide_string_list* com) {
    //wstrlist_debug_print(com);
    if (com->item_count >= 1) {
        Wide_string command = wstrlist_get(com, 0);
        if (wstrcmp(command.str, L"open", command.size, 5)) {

            if (comexpect_args(com, 1)) return;
            Wide_string file_name = wstrlist_get(com, 1);

            u8* resstr = wstrdgr(file_name.str, file_name.size);
            TBUFID id = tsFILE_open(resstr, file_name.size);
            free(resstr);
            if (TB_system_error != TBSE_OK) {
                comerror_file_not_found(&file_name);
                comerror_file_invalid();
                TB_system_error = TBSE_OK;
                return;
            }

            /*int chosen_buffer = TWIN1;
            ts_free_buffer(bwindows[chosen_buffer]->buf_id);
            bwindows[chosen_buffer]->buf_id = id;
            bwindow_buf_set_flags_on(bwindows[chosen_buffer], TB_UPDATED);*/
            fbw_add_entry(id, file_name.str, file_name.size);

        // TODO: consider making an alternative version that only takes the window ID (requires interaction with the FBW, select the buffer & enter)
        } else if (wstrcmp(command.str, L"bind", command.size, 5)) {

            if (comexpect_args(com, 2)) return;
            if (comexpect_argtype(com, 1, COMARGTYPE_INT)) return;
            if (comexpect_argtype(com, 2, COMARGTYPE_INT)) return;
            if (comexpect_argrange(com, 1, 1, 4)) return;
            if (comexpect_argrange(com, 2, NUMBER_OF_SYSTEM_WINDOWS, INT_MAX)) return;
            if (comexpect_false_in_array(com, 2, TB_system.free, TB_system.current_alloc)) return;

            Wide_string _winid = wstrlist_get(com, 1);
            Wide_string _bufid = wstrlist_get(com, 2);
            u32 winid = wstrtonum(_winid.str, _winid.size);
            u32 bufid = wstrtonum(_bufid.str, _bufid.size);

            if (comexpect_buffer_unbound(bufid)) return;

            bwindows[TWIN(winid)]->buf_id = bufid;
            bwindow_buf_set_flags_on(bwindows[TWIN(winid)], TB_UPDATED);

        } else if (wstrcmp(command.str, L"new", command.size, 4)) {

            TBUFID id = tsFILE_new();
            fbw_add_entry(id, STR_SYS_NEW_BUFFER.str, STR_SYS_NEW_BUFFER.size);

        } else if (wstrcmp(command.str, L"save", command.size, 5)) {

            if (comexpect_args(com, 1)) return;
            if (comexpect_argtype(com, 1, COMARGTYPE_INT)) return;
            if (comexpect_argrange(com, 1, NUMBER_OF_SYSTEM_WINDOWS, INT_MAX)) return;
            if (comexpect_false_in_array(com, 1, TB_system.free, TB_system.current_alloc)) return;
            Wide_string _bufid = wstrlist_get(com, 1);
            u32 bufid = wstrtonum(_bufid.str, _bufid.size);

            tsFILE_save(bufid); // TODO: Save error reporting

        } else {
            bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID);
            return;
        }
    } else {
        //resp.msg.str = L"No command given";
    }


    return;
}

bool comexpect_args(Wide_string_list* com, int numargs) {
    if (com->item_count - 1 != (u32)numargs) {
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

bool comexpect_argtype(Wide_string_list* com, u32 argnum, Command_argument_type argtype) {
    if (argtype == COMARGTYPE_INT) { // TODO: This can be made more efficient by cutting the wstrlist_get method and creating a wstrlist_isposnum or expanding it here
        Wide_string temp = wstrlist_get(com, argnum);
        if (wstrisnum(temp.str, temp.size)) {
            return false;
        } else {
            bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGTYPE);
            return true;
        }
    }
    return false;
}
bool comexpect_argrange(Wide_string_list* com, u32 argnum, int minimum, int maximum) { /* We expect the argument to be already an integer */
    Wide_string temp = wstrlist_get(com, argnum);
    u32 tempnum = wstrtonum(temp.str, temp.size);
    if (((u32)minimum <= tempnum) && (tempnum <= (u32)maximum)) {
        return false;
    } else {
        bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGRANGE);
        return true;
    }
}

bool comexpect_false_in_array(Wide_string_list* com, u32 argnum, bool* arr, u32 arrsize) {
    Wide_string temp = wstrlist_get(com, argnum);
    u32 tempnum = wstrtonum(temp.str, temp.size);

    if (tempnum < arrsize) {
        if (arr[tempnum]) {
            bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGRANGE);
            return true;
        } else {
            return false;
        }
    } else {
        bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGRANGE);
        return true;
    }
}

bool comexpect_true_in_array(Wide_string_list* com, u32 argnum, bool* arr, u32 arrsize) {
    Wide_string temp = wstrlist_get(com, argnum);
    u32 tempnum = wstrtonum(temp.str, temp.size);

    if (tempnum < arrsize) {
        if (!arr[tempnum]) {
            bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGRANGE);
            return true;
        } else {
            return false;
        }
    } else {
        bwindow_buf_insert_text(bwindows[TWINCOM], STR_COMMSG_INVALID_ARGRANGE);
        return true;
    }
}

bool comexpect_buffer_unbound(TBUFID bufid) {
    for (int i = TWIN4; i <= TWIN1; i++) {
        if (bwindows[i]->buf_id == bufid) {
            Wide_string msg = { 0 };
            int chars_to_alloc = _scwprintf(STR_COMMSG_BUFFER_ALREADY_BOUND.str, i) + 1; // _scwprintf doesn't count the terminating char
            msg.size = chars_to_alloc;
            msg.str = emalloc(chars_to_alloc * sizeof(wchar_t));
            _snwprintf_s(msg.str, chars_to_alloc, chars_to_alloc - 1, STR_COMMSG_BUFFER_ALREADY_BOUND.str, (int)(i - NUMBER_OF_SYSTEM_WINDOWS + 1));
            bwindow_buf_insert_text(bwindows[TWINCOM], msg);
            free(msg.str);
            return true;
        }
    }
    return false;
}


void comerror_file_not_found(Wide_string* file_name) {
    if (TB_system_error == TBSE_FILE_NOT_FOUND) {
        /* File not found <file_name>\n */
        Wide_string msg = { 0 };
        u32 s1_size = 0;
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
