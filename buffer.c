#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdarg.h>

#include "app.h"
#include "buffer.h"

#include "misc.h"
#include "wstr.h"
#include "databuffer.h"
#include "list.h"

#include "command.h"

void tbuffer_init(Text_buffer* buf, int in_size) {
    buf->b_size = in_size;
    buf->before_cursor = ecalloc(in_size, sizeof(wchar_t));
    buf->after_cursor = ecalloc(in_size, sizeof(wchar_t));
    buf->ac_current_char = in_size;
    buf->bc_current_char = 0;
    buf->current_chars_stored = 0;
    buf->flags = TB_WRITTABLE;
    buf->linked_file_path.size = 0;
    buf->linked_file_path.str = NULL;

    buf->render_lines.line_amount = 0; // Setting things to 0 is not needed because we calloced the struct but it's here anyways
    buf->render_lines.line_starts = NULL;
    buf->abs_mark_position = -1;
}

Text_buffer* tbuffer_create(int in_size) {
    Text_buffer* res = emalloc(sizeof(Text_buffer));
    tbuffer_init(res, in_size);
    return res;
}

Text_buffer* tbuffer_from_databuffer(Data_buffer* dat) {
    if (dat->cursize % 2 == 1) return NULL; // wchar_t is 2 bytes, so if we have an odd numbers of bytes in dat we can't get wchar_t right
    Text_buffer* res = emalloc(sizeof(Text_buffer)); // TODO: Use the tbuffer_init method
    res->b_size = dat->cursize/2;
    res->before_cursor = ecalloc(res->b_size, sizeof(wchar_t));
    res->after_cursor = ecalloc(res->b_size, sizeof(wchar_t));
    res->ac_current_char = res->b_size;
    res->bc_current_char = res->b_size - 1;
    res->current_chars_stored = res->b_size - 1;
    res->flags = TB_WRITTABLE | TB_UPDATED;

    res->render_lines.line_amount = 0;
    res->render_lines.line_starts = NULL;
    res->abs_mark_position = -1;

    memcpy(res->before_cursor, dat->data, sizeof(wchar_t)*(res->b_size - 1));
    return res;
}

bool tbuffer_insert(Text_buffer* buf, wchar_t c) {
    if (IS_FLAG_ON(buf->flags, TB_WRITTABLE)) {
        if (buf->current_chars_stored >= buf->b_size) {
            tbuffer_resize(buf);
        }
        buf->before_cursor[buf->bc_current_char] = c;
        buf->current_chars_stored++;
        buf->bc_current_char++;

        buf->flags |= TB_UPDATED;
        return true;
    }
    return false;
}

void tbuffer_insert_bypass(Text_buffer* buf, wchar_t c) {
    if (buf->current_chars_stored >= buf->b_size) {
        tbuffer_resize(buf);
    }
    buf->before_cursor[buf->bc_current_char] = c;
    buf->current_chars_stored++;
    buf->bc_current_char++;

    buf->flags |= TB_UPDATED;
}

void tbuffer_insert_string_bypass(Text_buffer* buf, wchar_t* str, int sz) {
    if (buf->current_chars_stored + sz - 1 >= buf->b_size) {
        tbuffer_resize_custom(buf, sz);
    }
    memcpy(buf->before_cursor + buf->bc_current_char, str, sizeof(wchar_t)*(sz - 1)); // We don't want to copy the terminator
    buf->current_chars_stored += sz;
    buf->bc_current_char += sz - 1;

    buf->flags |= TB_UPDATED;
}

void tbuffer_insert_formatted_bypass(Text_buffer* buf, const wchar_t* formatstring, ...) {
    va_list args;
    va_start(args, formatstring);

    Wide_string msg = { 0 };
    int chars_to_alloc = _vscwprintf(formatstring, args) + 1; // _vscwprintf doesn't count the terminating char
    msg.size = chars_to_alloc;
    msg.str = emalloc(chars_to_alloc * sizeof(wchar_t));
    _vsnwprintf_s(msg.str, chars_to_alloc, chars_to_alloc - 1, formatstring, args); // _vsnwprintf_s is probably the worst name I have seen for a function
    // variadic store new wide print formatted secure
    tbuffer_insert_string_bypass(buf, msg.str, msg.size);

    free(msg.str);
    va_end(args);
}

bool tbuffer_backspace(Text_buffer* buf) {
    if (buf->bc_current_char != 0) {
        buf->before_cursor[buf->bc_current_char-1] = 0;
        buf->bc_current_char--;
        buf->current_chars_stored--;
        buf->flags |= TB_UPDATED;
        return true;
    }
    return false;
}

void tbuffer_resize(Text_buffer* buf) {
    int old_size = buf->b_size;
    buf->b_size *= 2;

    // Copy before
    wchar_t* new_before = ecalloc(buf->b_size, sizeof(wchar_t));
    memcpy(new_before, buf->before_cursor, sizeof(wchar_t)*buf->bc_current_char);
    free(buf->before_cursor);
    buf->before_cursor = new_before;

    // Copy after. We need the contents of the buffer to still start from the end.
    wchar_t* new_after = ecalloc(buf->b_size, sizeof(wchar_t));
    memcpy(new_after + old_size, buf->after_cursor, sizeof(wchar_t)*old_size); // We add old_size bcs buf->b_size - old_size = old_size
    free(buf->after_cursor);
    buf->after_cursor = new_after;

    buf->ac_current_char += buf->b_size - old_size;
}

void tbuffer_resize_custom(Text_buffer* buf, int sz) {
    int old_size = buf->b_size;
    buf->b_size += sz + 1;

    // Copy before
    wchar_t* new_before = ecalloc(buf->b_size, sizeof(wchar_t));
    memcpy(new_before, buf->before_cursor, sizeof(wchar_t)*buf->bc_current_char);
    free(buf->before_cursor);
    buf->before_cursor = new_before;

    // Copy after. We need the contents of the buffer to still start from the end.
    wchar_t* new_after = ecalloc(buf->b_size, sizeof(wchar_t));
    memcpy(new_after + (buf->b_size - old_size), buf->after_cursor, sizeof(wchar_t)*old_size); /* Please make sure you actually know the algorithms before
    implementing them */
    free(buf->after_cursor);
    buf->after_cursor = new_after;

    buf->ac_current_char += buf->b_size - old_size;
}

int tbuffer_move_cursor(Text_buffer* buf, int amount) {
    if (amount < 0) {
        amount = -amount;
        int actual = mini(amount, buf->bc_current_char);
        buf->bc_current_char -= actual;
        buf->ac_current_char -= actual; // minus bcs it's flipped.

        memcpy(buf->after_cursor + buf->ac_current_char, buf->before_cursor + buf->bc_current_char, sizeof(wchar_t)*actual);
        memset(buf->before_cursor + buf->bc_current_char, 0, sizeof(wchar_t)*actual);

        buf->flags |= TB_UPDATED;
        return actual;
    } else if (amount > 0) {
        int actual = amount;
        if (amount+buf->ac_current_char > buf->b_size) actual = buf->b_size - buf->ac_current_char;
        memcpy(buf->before_cursor + buf->bc_current_char, buf->after_cursor + buf->ac_current_char, sizeof(wchar_t)*actual);
        memset(buf->after_cursor + buf->ac_current_char, 0, sizeof(wchar_t)*actual);

        buf->bc_current_char += actual;
        buf->ac_current_char += actual; // plus bcs it's flipped.

        buf->flags |= TB_UPDATED;
        return actual;
    }
    return 0;
}

void tbuffer_move_cursor_to_pos(Text_buffer* buf, int pos) {
    tbuffer_move_cursor(buf, pos - buf->bc_current_char - (buf->bc_current_char > pos ? 0 : 1) );
}

void tbuffer_clear(Text_buffer* buf) {
    memset(buf->before_cursor, 0, buf->b_size*sizeof(wchar_t));
    memset(buf->after_cursor, 0, buf->b_size*sizeof(wchar_t));
    buf->current_chars_stored = 0;
    buf->bc_current_char = 0;
    buf->ac_current_char = buf->b_size;

    buf->flags |= TB_UPDATED;
}

/* We store the positions of near newlines in a Render_lines object to render them char by char later to
   make it easier to color them. */
void tbuffer_render(Buffer_window* bwin, Text_buffer* buf, int* cy, int* cx) {

    // TODO: Ensure we are using memory correctly
    WINDOW* win = bwin->curses_window;
    int winh = getmaxy(win);
    int winw = getmaxx(win);
    int center = winh % 2 == 0 ? winh/2 : (winh-1)/2;
    int line_offset = 0;
    bool endscroll = IS_FLAG_ON(buf->flags, TB_ENDSCROLL);

    if (endscroll) {
        center = winh - 1;
    }

    // Reset the render lines
    Render_lines* rlines = &buf->render_lines;
    if (rlines->line_amount != winh) {
        if (rlines->line_starts != NULL) free(rlines->line_starts);
        rlines->line_starts = ecalloc(winh+1, sizeof(int));
        rlines->line_amount = winh;
    }
    int i = 0;
    for (; i <= rlines->line_amount; i++) {
        rlines->line_starts[i] = -1;
    }

    // before_cursor and first part of the current line
    for (i = buf->bc_current_char - 1; i >= 0; i--) {
        if ((buf->before_cursor[i] == '\n') || (i == 0)) {
            rlines->line_starts[center+line_offset] = i;
            line_offset--;
        }

        if (center+line_offset < 0) break;
    }

    line_offset = 1;
    // after_cursor and second part of the current line
    for (i = buf->bc_current_char; i <= buf->current_chars_stored; i++) { // TODO: is <= buf->current_chars_stored problematic??????¿?¿
        if ((tbuffer_get_char_absolute(buf, i) == '\n') || (i == buf->current_chars_stored)) {
            rlines->line_starts[center+line_offset] = i;
            line_offset++;
        }

        if (tbuffer_get_char_absolute(buf, i) == '\0') break;
        if (center+line_offset > winh) break;
    }

    werase(win);
    wmove(win, 0, 0);
    for (i = 0; i < rlines->line_amount; i++) {
        int start = rlines->line_starts[i];
        int end = rlines->line_starts[i + 1];

        if ( (end < 0) || (start < 0) || (end < start)) continue;
        wmove(win, i, 0);

        /*int c = start == 0 ? start : start + 1;
        for (; c < end; c++) {
            u32 char_to_add = 0; // This is treated as a PDCurses chtype/cchar_t
            // Pointer math incoming
            *(((u16*)&char_to_add)) = tbuffer_get_char_absolute(buf, c); //*(((u16*)&char_to_add) + 1) doesn't work. This apparently depends on endianness TODO
            wadd_wch(win, (cchar_t*)&char_to_add);
        }*/
        if (i < center) {
            waddnwstr(win, buf->before_cursor + start + (start == 0 ? 0 : 1), end - start); // Apparently this function is like really expensive
            // Today I learned the windows console is really badly optimized
        }
        if (i > center) {
            waddnwstr(win, buf->after_cursor + buf->ac_current_char + start + (start == 0 ? 0 : 1), end - start);
        }
    }

    wrefresh(win);

    // Cursor position
    if ((cx != NULL) && (cy != NULL)) {
        *cy = center;
        *cx = rlines->line_starts[center] == 0 ?
            buf->bc_current_char-rlines->line_starts[center]   :
            buf->bc_current_char-rlines->line_starts[center]-1 ; // TODO: check
    }
}

// TODO: we can change this to just use a memcpy after we changed the type of the buffes to wchar_t*
wchar_t* tbuffer_translate_string(Text_buffer* buf, Buffer_order b, int st, int en) {
    if (b == BO_BEFORE) {
        if (st < 0) st = 0;
        if (en > buf->bc_current_char) en = buf->bc_current_char;
        wchar_t* beforestr = ecalloc(en - st + 1, sizeof(wchar_t));
        for (int i = st; i < en; i++) {
            beforestr[i - st] = buf->before_cursor[i];
        }
        return beforestr;
    }

    if (st < buf->ac_current_char) st = buf->ac_current_char;
    if (en > buf->b_size) en = buf->b_size;
    wchar_t* afterstr = ecalloc(en - st + 1, sizeof(wchar_t));
    for (int i = st; i < en; i++) {
        afterstr[i - st] = buf->after_cursor[i];
    }
    return afterstr;

}

int tbuffer_last_nl_before(Text_buffer* buf, int pos) {
    pos = mini(pos, buf->bc_current_char);
    for (int i = 0; i <= pos; i++) {
        if ((buf->before_cursor[pos-i] == '\n') || (i == pos)) {
            return pos-i;
        }
    }
    return 0; //Compiler complains if this isn't here
}

int tbuffer_first_nl_after(Text_buffer* buf, int pos) {
    pos = maxi(mini(pos, buf->b_size), buf->ac_current_char);
    for (int i = 0; i <= buf->b_size - pos; i++) {
        if ((buf->after_cursor[pos+i] == '\n') || (i == buf->b_size - pos)) {
            return pos+i;
        }
    }
    return buf->b_size;
}

int tbuffer_find_line(Text_buffer* buf, int l) {
    for (int i = 0; i < buf->bc_current_char; i++) { // Check first for the after_cursor
        if (buf->before_cursor[i] == L'\n') l--;
        if (l == 1) return i;
    }

    for (int i = buf->ac_current_char; i < buf->b_size; i++) {
        if (buf->after_cursor[i] == L'\n') l--;
        if (l == 1) return i;
    }
    return -1;
}

int tbuffer_get_cursor_line(Text_buffer* buf) {
    int l = 0;
    wchar_t* bc = buf->before_cursor;
    for (int i = 0; i < buf->bc_current_char; i++) {
        if (bc[i] == '\n') l++;
    }
    return l;
}

wchar_t tbuffer_get_char_absolute(Text_buffer* buf, int pos) {
    if ((pos >= buf->current_chars_stored) || (pos < 0)) return L'\0';
    if (pos >= buf->bc_current_char) { // Return from the after_cursor buffer
        return buf->after_cursor[pos - buf->bc_current_char + buf->ac_current_char];
    }
    // Return from the before_cursor buffer
    return buf->before_cursor[pos];
}

void tbuffer_free(Text_buffer* buf) {
    free(buf->after_cursor);
    free(buf->before_cursor);
    free(buf);
}

/*
    BUFFER SYSTEM STUFF
*/

void ts_start() {
    TB_system.current_alloc = 3;
    TB_system.buffers = ecalloc(TB_system.current_alloc, sizeof(Text_buffer));
    TB_system.free = ecalloc(TB_system.current_alloc, sizeof(bool));
    for (u32 i = 0; i < TB_system.current_alloc; i++) {
        //tbuffer_init(TB_system.buffers + i, 100);
        TB_system.free[i] = true;
    }
    TB_system_error = TBSE_OK;

}

void ts_shutdown() {
    // TODO: maybe saved closed files?????????????
    for (u32 i = 0; i < TB_system.current_alloc; i++) {
        if (!TB_system.free[i]) {
            free(TB_system.buffers[i].after_cursor);
            free(TB_system.buffers[i].before_cursor);
        }
    }
    free(TB_system.buffers);
    free(TB_system.free);
}

// Returns the ID of the uninited buffer
TBUFID ts_ensure_free() {
    for (u32 i = 0; i < TB_system.current_alloc; i++) {
        if (TB_system.free[i]) {
            TB_system.free[i] = false;
            return i;
        }
    }
    TB_system.buffers = erealloc(TB_system.buffers, sizeof(Text_buffer)*TB_system.current_alloc*2);
    TB_system.free = erealloc(TB_system.free, sizeof(bool)*TB_system.current_alloc*2);
    for (u32 i = TB_system.current_alloc; i < TB_system.current_alloc*2; i++) {
        //tbuffer_init(TB_system.buffers + i, 100);
        TB_system.free[i] = true;
    }

    TB_system.current_alloc *= 2;
    TB_system.free[TB_system.current_alloc/2] = false;
    return TB_system.current_alloc/2;
}

void ts_free_buffer(TBUFID id) {
    if (id < TB_system.current_alloc) {
        if (!TB_system.free[id]) {
            TB_system.free[id] = true;
            Text_buffer b = TB_system.buffers[id];
            free(b.after_cursor);
            free(b.before_cursor);
        } else {
            TB_system_error = TBSE_ALREADY_FREE;
        }
    } else {
        TB_system_error = TBSE_OUT_OF_BOUNDS;
    }
}

TBUFID tsFILE_new() {
    TBUFID id = ts_ensure_free();
    TB_system.free[id] = false;
    /*free(buf.after_cursor);
    free(buf.before_cursor);*/
    tbuffer_init(&TB_system.buffers[id], 100);
    return id;
}

TBUFID tsFILE_open(const u8* name, u32 name_size) { // TODO: Make sure we don't load any illegal files (unsupported UNICODE, etc...) & we don't open already opened files
    // TODO: We could have the file streaming into a data buffer and turning it into UTF16 at the same time with threads if performance is important
    FILE* f = fopen((char*)name, "r"); // There's a fopen variant called _wfopen_s that can open files with wchar_t names
    if (f == NULL) {
        TB_system_error = TBSE_FILE_NOT_FOUND;
        return 2000000;
    }
    Data_buffer* dat = databuffer_new(100);
    int c = 0;
    while (true) {
        c = fgetc(f);
        if (feof(f)) break;
        databuffer_add_byte(dat, (u8)c);
    }
    databuffer_add_byte(dat, '\0');

    Data_buffer* utf16 = databuffer_new(10);
    if (utftrans_8to16(dat->data, dat->cursize, utf16)) {
        TB_system_error = TBSE_INVALID_FILE;
        return 2000000;
    }

    if (utf16->cursize % 2 == 1) {
        TB_system_error = TBSE_INVALID_FILE;
        return 2000000;
    }
    TBUFID id = ts_ensure_free();
    Text_buffer* buf = &(TB_system.buffers[id]);
    buf->b_size = utf16->cursize/2;
    buf->before_cursor = ecalloc(buf->b_size, sizeof(wchar_t));
    buf->after_cursor = ecalloc(buf->b_size, sizeof(wchar_t));
    buf->ac_current_char = buf->b_size;
    buf->bc_current_char = buf->b_size - 1;
    buf->current_chars_stored = buf->b_size - 1;
    buf->flags = TB_WRITTABLE | TB_UPDATED;
    buf->linked_file_path.str = ecalloc(name_size, sizeof(u8));
    buf->linked_file_path.size = name_size;

    memcpy(buf->linked_file_path.str, name, sizeof(u8)*name_size);
    memcpy(buf->before_cursor, utf16->data, sizeof(wchar_t)*(buf->b_size - 1));
    databuffer_free(utf16);
    databuffer_free(dat);

    return id;
}

void tsFILE_save(TBUFID buf) {
    // Check the buffer has a bound file path (TODO: Make sure it's a valid filepath)
    Text_buffer* b = &TB_system.buffers[buf];
    if (b->linked_file_path.size == 0) {
        TB_system_error = TBSE_INVALID_PATH;
        return;
    }

    Narrow_string temp_file_path = { 0 };
    temp_file_path.str = nstrcat( // Concatenate b->linked_file_path & ".longextensionsothatnofilesareoverwrittenctez"
        b->linked_file_path.str,
        STR_TEMPORAL_FILE_EXTENSION.str,
        b->linked_file_path.size,
        STR_TEMPORAL_FILE_EXTENSION.size,
        &temp_file_path.size);

    // We first write to a temporal file to ensure the original file is not going to be corrupted, or if it is there's a backup for it
    FILE* tempfile;
    tempfile = fopen((char*)temp_file_path.str, "w+"); // TODO: make sure that the cast in here doesn't cause problems with weird file names

    if (tempfile == NULL) { // If we can't create this file wtf are we doing
        TB_system_error = TBSE_CANT_CREATE_FILE;
        return;
    }

    Data_buffer* dat_before = utftrans_16to8(b->before_cursor, b->bc_current_char);
    Data_buffer* dat_after = utftrans_16to8(b->after_cursor + b->ac_current_char, b->b_size - b->ac_current_char);

    for(u64 i = 0; i < dat_before->cursize; i++) {
        fputc(dat_before->data[i], tempfile);
    }

    for(u64 i = 0; i < dat_after->cursize; i++) {
        fputc(dat_after->data[i], tempfile);
    }

    fclose(tempfile);

    // We now write to the original file, with a copy having been made so that we ensure no data is lost
    FILE* actualfile;
    actualfile = fopen((char*)b->linked_file_path.str, "w+");

    if (actualfile == NULL) {
        TB_system_error = TBSE_FILE_NOT_FOUND;
        return;
    }

    for(u64 i = 0; i < dat_before->cursize; i++) {
        fputc(dat_before->data[i], actualfile);
    }

    for(u64 i = 0; i < dat_after->cursize; i++) {
        fputc(dat_after->data[i], actualfile);
    }

    fclose(actualfile);

    // Delete the backup file now that we have ensured the original file has the correct data (TODO: check or smth) and free what we have used
    remove((char*)temp_file_path.str);

    databuffer_free(dat_before);
    databuffer_free(dat_after);
    free(temp_file_path.str);
}

/*
FILE* f;
f = fopen("test.txt", "w+");
Data_buffer* dat = utftrans_16to8(main_buffer->before_cursor, main_buffer->bc_current_char);
for(u64 i = 0; i < dat->cursize; i++) {
    fputc(dat->data[i], f);
}
fclose(f);
databuffer_free(dat);
*/

/*
    BUFFER WINDOW STUFF
*/

Buffer_window* bwindow_create() {
    Buffer_window* b = ecalloc(1, sizeof(Buffer_window));
    b->buf_id = tsFILE_new();
    b->flags = 0;
    b->win_id = -1;
    return b;
}

void bwindow_handle_keypress(Buffer_window* w, int key, u64 key_mods) {

    // 443 -> ctrl left, 444 -> ctrl right, 480 -> ctrl up, 481 -> ctrl down
    Text_buffer* buf = &TB_system.buffers[w->buf_id];
    bool is_comline = IS_FLAG_ON(buf->flags, TB_COMLINE);
    bool is_writtable = IS_FLAG_ON(buf->flags, TB_WRITTABLE);

    // The checks for mod keys inside key checks are because it's possible for modified keys to overlap with special keys (ej.: ctrl+h = backspace)
    switch (key) {
    case 13:
    case PADENTER:
        if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break;
        if (is_comline) {
            Wide_string command = { 0 };
            command.str = wstrcat_second_no_terminator(
                    buf->before_cursor,
                    buf->after_cursor + buf->ac_current_char,
                    buf->bc_current_char + 1, // wstrcat expects the first string's size to also include the terminator
                    buf->b_size - buf->ac_current_char,
                    &command.size);
            Wide_string_list* com = command_parse(command.str, command.size); // TODO: enter in the middle of a command the command gets executed
            command_execute(com);
            buf = &TB_system.buffers[w->buf_id]; // We get the buffer again because command_execute might have reallocated the buffers array
            tbuffer_clear(buf);
            free(com);
            free(command.str);
        }
        else if (is_writtable) tbuffer_insert(buf, '\n');
        break;

    case 9:
        if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break;
        if ((!is_comline) && (is_writtable)) tbuffer_insert(buf, '\t');
        break;

    case 8: // Backspace
        if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break;
        if (is_writtable) tbuffer_backspace(buf);
        break;

    case PADSTAR: if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break; if (is_writtable) tbuffer_insert(buf, '*'); break;
    case PADSLASH: if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break; if (is_writtable) tbuffer_insert(buf, '/'); break;
    case PADPLUS: if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break; if (is_writtable) tbuffer_insert(buf, '+'); break;
    case PADMINUS: if ( (key_mods & (PDC_KEY_MODIFIER_CONTROLALT)) != 0) break; if (is_writtable) tbuffer_insert(buf, '-'); break;

    case KEY_LEFT:
        tbuffer_move_cursor(buf, -1);
        break;

    case KEY_RIGHT:
        tbuffer_move_cursor(buf, 1);
        break;

    case KEY_UP: // TODO: If lines only consist of \n some weird things happen, like an infinite loop (wtf). Maybe use the Render_lines
        if (is_comline) return;
        int linestart = tbuffer_last_nl_before(buf, buf->bc_current_char);
        if (linestart != 0) {
            int newlinestart = tbuffer_last_nl_before(buf, linestart - 1);
            linestart = newlinestart == 0 ? linestart + 1 : linestart;
            if (linestart - newlinestart < buf->bc_current_char - linestart) {
                tbuffer_move_cursor(buf, linestart - buf->bc_current_char);
            } else {
                tbuffer_move_cursor(buf, newlinestart - linestart);
            }
        }
        break;

    case KEY_DOWN:
        if (is_comline) return;
        // This is code
        int lineend = tbuffer_first_nl_after(buf, buf->ac_current_char) - buf->ac_current_char + buf->bc_current_char;
        // (- buf->ac_current_char + buf->bc_current_char) translates it from after_cursor coord space to before_cursor coord space
        if (lineend == buf->b_size - 1) {
            tbuffer_move_cursor(buf, buf->b_size - buf->bc_current_char);
        } else {
            int linestart = tbuffer_last_nl_before(buf, buf->bc_current_char);
            int newlineend = tbuffer_first_nl_after(buf, lineend - buf->bc_current_char + buf->ac_current_char + 1)
                             - buf->ac_current_char + buf->bc_current_char;
            linestart = linestart == 0 ? -1 : linestart;
            newlineend = newlineend == buf->b_size - 1 ? newlineend - 1 : newlineend;
            if (buf->bc_current_char - linestart > newlineend - lineend) { // Current line is bigger than following line
                tbuffer_move_cursor(buf, newlineend - buf->bc_current_char);
            } else {
                tbuffer_move_cursor(buf, - linestart + lineend);
            }
        }
        break;

    case CTL_LEFT:
        if (wchrissymbol(buf->before_cursor[buf->bc_current_char - 1])) {
            while (buf->bc_current_char != 0) {
                if (!wchrissymbol(buf->before_cursor[buf->bc_current_char - 1])) break;
                tbuffer_move_cursor(buf, -1);
            }
        }
        while (buf->bc_current_char != 0) {
            if (wchrissymbol(buf->before_cursor[buf->bc_current_char - 1])) break;
            tbuffer_move_cursor(buf, -1);
        }
        break;

    case CTL_RIGHT:
        if (wchrissymbol(buf->after_cursor[buf->ac_current_char + 1])) {
            while (buf->ac_current_char != buf->b_size - 1) {
                if (!wchrissymbol(buf->after_cursor[buf->ac_current_char + 1])) break;
                tbuffer_move_cursor(buf, 1);
            }
        }
        while (buf->ac_current_char != buf->b_size - 1) {
            if (wchrissymbol(buf->after_cursor[buf->ac_current_char + 1])) break;
            tbuffer_move_cursor(buf, 1);
        }
        break;

    case CTL_ENTER:
        buf->abs_mark_position = buf->bc_current_char;
        break;

    }

    if (((key >= 32 && key <= 0x7E) || (key >= 0xA1 && key <= 0xFF)) && (is_writtable))  {
        tbuffer_insert(buf, key);
    }
}

void bwindow_buf_set_flags_on(Buffer_window* w, u8 flags) {
    TB_system.buffers[w->buf_id].flags |= flags;
}

void bwindow_buf_set_flags_off(Buffer_window* w, u8 flags) {
    SET_FLAG_OFF(TB_system.buffers[w->buf_id].flags, flags);
}

void bwindow_buf_insert_text(Buffer_window* w, Wide_string str) {
    if (w->buf_id >= TB_system.current_alloc) {
        TB_system_error = TBSE_OUT_OF_BOUNDS;
        return;
    }
    Text_buffer* buf = &TB_system.buffers[w->buf_id];
    if (IS_FLAG_ON(buf->flags, TB_COMOUTPUT)) { // Make sure the cursor is at the end when inserting text onto the read only console
        tbuffer_move_cursor(buf, buf->current_chars_stored - buf->bc_current_char);
    }
    tbuffer_insert_string_bypass(buf, str.str, str.size);
}

void bwindow_update(Buffer_window* w, int winh, int* cursorx, int* cursory, bool is_selected_window) {
    Text_buffer* curbuffer = &(TB_system.buffers[w->buf_id]);
    if (IS_FLAG_ON(curbuffer->flags, TB_UPDATED)) {
        SET_FLAG_OFF(curbuffer->flags, TB_UPDATED); // TODO: If we want to make multiple windows being bound to one buffer we have to do this at the end
                                                    //       of the main loop
        /* We return the current array of lines from the function to free them after we have already set the new lines.
           This is because addwstr apparently needs the pointer alive and doesn't copy the contents of the string? Idk */
        tbuffer_render(w, curbuffer, cursory, cursorx);

        if (is_selected_window) {
            // Render the line number
            wchar_t* line_number = wstrfromnum(tbuffer_get_cursor_line(curbuffer) + 1, NULL);

            for (int i = 1; i < 8; i++) {
                mvaddch(winh - BOTTOM_SECTION_HEIGHT, i, 0x2501);
            }

            mvaddwstr(winh - BOTTOM_SECTION_HEIGHT, 1, line_number);
            free(line_number);
        }
    }
}

/*
    FILE BUFFER WINDOW FUNCTIONS
*/

void fbw_start(TBUFID fb) {
    list_init(&App.fbw.ids, 100);
    list_init(&App.fbw.binds, 100);
    wstrlist_init(&App.fbw.file_names, 100);
    wstrlist_init(&App.fbw.names, 100);
    App.fbw.fb = fb;
}

void fbw_shutdown() {
    free(App.fbw.names.data);
    free(App.fbw.names.locations);
    free(App.fbw.file_names.data);
    free(App.fbw.file_names.locations);
    free(App.fbw.ids.data);
    free(App.fbw.ids.locations);
}

u32 fbw_add_entry(TBUFID newbufid, const wchar_t* path, u32 pathsz, int* winid) {
    u32 file_name_size = 0;
    wchar_t* file_name = wstrfilefrompath(path, pathsz, &file_name_size);

    wstrlist_add(&App.fbw.file_names, path, pathsz); // This is named in an stupid way
    wstrlist_add(&App.fbw.names, file_name, file_name_size);
    list_add(&App.fbw.ids, &newbufid, sizeof(TBUFID));
    if (winid == NULL) {
        int tempval = -1;
        list_add(&App.fbw.binds, &tempval, sizeof(int));
    } else list_add(&App.fbw.binds, winid, sizeof(int));

    // Insert the string in the correct buffers
    Text_buffer* buf = &TB_system.buffers[App.fbw.fb];

    tbuffer_move_cursor(buf, buf->current_chars_stored - buf->bc_current_char);
    tbuffer_insert_formatted_bypass(buf, L"%d %s\n", (int)newbufid, file_name);

    free(file_name);
    /*free(label);
    free(_label);
    free(_bufid);
    free(_bufidstr);*/

    return App.fbw.ids.item_count;
}

int fbw_find_entry(TBUFID id) {
    for (u32 i = 0; i < App.fbw.ids.item_count; i++) {
        Sized_pointer o = list_get(&App.fbw.ids, i);
        if ( *((TBUFID*)o.data) == id) return (int)i;
    }
    return -1;
}

// TODO: check bounds
void fbw_mark_bind(TBUFID id, int win_id) {
    Text_buffer* buf = &TB_system.buffers[App.bwindows[TWINFILE]->buf_id];
    int entry_num = fbw_find_entry(id);
    if (entry_num == -1) return;
    Wide_string file_name = wstrlist_get(&App.fbw.names, entry_num);

    tbuffer_move_cursor_to_pos(buf, tbuffer_find_line(buf, entry_num+2));
    while ((buf->before_cursor[buf->bc_current_char - 1] != L'\n') && (buf->bc_current_char != 0)) {
        tbuffer_backspace(buf);
    }
    tbuffer_insert_formatted_bypass(buf, L"%d %s (%d)", (int)id, file_name.str, MAX_WINDOWS - win_id);
    tbuffer_move_cursor_to_pos(buf, buf->current_chars_stored);

    buf->flags |= TB_UPDATED;
}

void fbw_mark_unbind(TBUFID id) {
    Text_buffer* buf = &TB_system.buffers[App.bwindows[TWINFILE]->buf_id];
    int entry_num = fbw_find_entry(id);
    if (entry_num == -1) return;
    Wide_string file_name = wstrlist_get(&App.fbw.names, entry_num);

    tbuffer_move_cursor_to_pos(buf, tbuffer_find_line(buf, entry_num+2));
    while ((buf->before_cursor[buf->bc_current_char - 1] != L'\n') && (buf->bc_current_char != 0)) {
        tbuffer_backspace(buf);
    }
    tbuffer_insert_formatted_bypass(buf, L"%d %s", (int)id, file_name.str);
    tbuffer_move_cursor_to_pos(buf, buf->current_chars_stored);

    buf->flags |= TB_UPDATED;
}
