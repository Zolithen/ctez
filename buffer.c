#include "buffer.h"
#include "misc.h"
#include "command.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

void tbuffer_init(Text_buffer* buf, int in_size) {
    buf->b_size = in_size;
    buf->before_cursor = ecalloc(in_size, sizeof(wchar_t));
    buf->after_cursor = ecalloc(in_size, sizeof(wchar_t));
    buf->ac_current_char = in_size;
    buf->bc_current_char = 0;
    buf->current_chars_stored = 0;
    buf->flags = TB_WRITTABLE;
}

Text_buffer* tbuffer_create(int in_size) {
    Text_buffer* res = emalloc(sizeof(Text_buffer));
    tbuffer_init(res, in_size);
    return res;
}

Text_buffer* tbuffer_from_databuffer(Data_buffer* dat) { // TODO: check if not copying the \0 is problematic
    if (dat->cursize % 2 == 1) return NULL; // wchar_t is 2 bytes, so if we have an odd numbers of bytes in dat we can't get wchar_t right
    Text_buffer* res = emalloc(sizeof(Text_buffer));
    res->b_size = dat->cursize/2;
    res->before_cursor = ecalloc(res->b_size, sizeof(wchar_t));
    res->after_cursor = ecalloc(res->b_size, sizeof(wchar_t));
    res->ac_current_char = res->b_size;
    res->bc_current_char = res->b_size - 1;
    res->current_chars_stored = res->b_size - 1;
    res->flags = TB_WRITTABLE | TB_UPDATED;

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

bool tbuffer_insert_string_bypass(Text_buffer* buf, wchar_t* str, int sz) {
    if (buf->current_chars_stored + sz - 1 >= buf->b_size) {
        tbuffer_resize_custom(buf, sz); // TODO: sometimes we need more than size*2 to store the new string. Somehow the other resizing method crashes after a while
    }
    memcpy(buf->before_cursor + buf->bc_current_char, str, sizeof(wchar_t)*(sz - 1)); // We don't want to copy the terminator
    buf->current_chars_stored += sz;
    buf->bc_current_char += sz - 1;

    buf->flags |= TB_UPDATED;

    return true;
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

void tbuffer_resize_custom(Text_buffer* buf, int sz) { // TODO: doesn't work
    int old_size = buf->b_size;
    buf->b_size += sz + 1;

    // Copy before
    wchar_t* new_before = ecalloc(buf->b_size, sizeof(wchar_t));
    memcpy(new_before, buf->before_cursor, sizeof(wchar_t)*buf->bc_current_char);
    free(buf->before_cursor);
    buf->before_cursor = new_before;

    // Copy after. We need the contents of the buffer to still start from the end.
    wchar_t* new_after = ecalloc(buf->b_size, sizeof(wchar_t));
    memcpy(new_after + (buf->b_size - old_size), buf->after_cursor, sizeof(wchar_t)*old_size);
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

/* We start iterating over chars that are near the cursor, so we find line breaks and
   get the amount of chars between them so we can render lines correctly */
void tbuffer_render(WINDOW* win, Text_buffer* buf, Lines_buffer* previous_lines, int* cy, int* cx) {

    int winh = getmaxy(win);
    int winw = getmaxx(win);
    int center = winh % 2 == 0 ? winh/2 : (winh-1)/2;
    int line_offset = 0;
    wchar_t** lines = ecalloc(winh, sizeof(wchar_t*)); // this one used to crash using the resize custom above
    bool*     lines_hasinit = ecalloc(winh, sizeof(bool));
    int*      lines_size = ecalloc(winh, sizeof(int)); // Wrap size
    int c_linebef_size = 0;
    int start_offset = 0;
    bool endscroll = IS_FLAG_ON(buf->flags, TB_ENDSCROLL);

    if (endscroll) {
        center = winh - 1;
    }

    // before_cursor and first part of the current line
    for (int i = 0; i <= buf->bc_current_char; i++) {
        if (buf->before_cursor[buf->bc_current_char - i] == '\n' ) {
            wchar_t* str = tbuffer_translate_string(buf, BO_BEFORE, buf->bc_current_char - i + 1, buf->bc_current_char - start_offset);
            lines[center+line_offset] = str;
            lines_hasinit[center+line_offset] = true;
            //buf->bc_current_char - start_offset - (buf->bc_current_char - i + 1)
            lines_size[center+line_offset] = (int)floor((i - 1 - start_offset)/winw);

            if (line_offset == 0) {
                int a = i - 1 + start_offset;
                if (a == -1) c_linebef_size = 1; // When backspacing a newline, the program crashed bcs c_linebef_size = -1. This should fix it
                else c_linebef_size = a;
            }

            line_offset--;
            start_offset = i - 1;
        } else if (i == buf->bc_current_char) { // The first line needs special treatment bcs it's not delimited with \n
            wchar_t* str = tbuffer_translate_string(buf, BO_BEFORE, buf->bc_current_char - i, buf->bc_current_char - start_offset);
            lines[center+line_offset] = str;
            lines_hasinit[center+line_offset] = true;
            lines_size[center+line_offset] = (int)floor((i - 1 - start_offset)/winw);

            if (line_offset == 0) c_linebef_size = i - start_offset;

            start_offset = i - 1;
            line_offset--;
        }
        if (center+line_offset < 0) break;
    }

    // after_cursor
    line_offset = 0;
    start_offset = 0;
    wchar_t* current_line_after = NULL;
    int c_lineaf_size = 0;
    for (int i = 0; i <= buf->b_size - buf->ac_current_char; i++) {
        if (buf->after_cursor[buf->ac_current_char + i] == '\n') {
            if (line_offset != 0) { // We need to handle this case differently bcs we grabbed the first part of the current line in before
                wchar_t* str = tbuffer_translate_string(buf, BO_AFTER, buf->ac_current_char + start_offset, buf->ac_current_char + i);
                lines[center+line_offset] = str;
                lines_hasinit[center+line_offset] = true;
                lines_size[center+line_offset] = (int)floor((i - start_offset)/winw);
            } else {
                current_line_after = ecalloc(i - start_offset, sizeof(wchar_t));
                memcpy(current_line_after, buf->after_cursor + buf->ac_current_char + start_offset, (i - start_offset)*sizeof(wchar_t));
                c_lineaf_size = i - start_offset;
            }
            if (endscroll) break;

            start_offset = i+1;
            line_offset++;
        } else if (i == buf->b_size - buf->ac_current_char) { // The last line needs special treatment bcs it's not delimited with \n
            if (line_offset != 0) { // Again, we handle this later
                wchar_t* str = tbuffer_translate_string(buf, BO_AFTER, buf->ac_current_char + start_offset, buf->ac_current_char + i + 1);
                lines[center+line_offset] = str;
                lines_hasinit[center+line_offset] = true;
                lines_size[center+line_offset] = (int)floor((i + 1 - start_offset)/winw);
            } else {
                current_line_after = ecalloc(i - start_offset, sizeof(wchar_t));
                memcpy(current_line_after, buf->after_cursor + buf->ac_current_char + start_offset, (i - start_offset)*sizeof(wchar_t)); //ok
                c_lineaf_size = i - start_offset;
            }
            if (endscroll) break;

            start_offset = i+1;
            line_offset++;
        }
        if (center+line_offset > winh-1) break;
    }

    // Correctly get the current line. We have the first part in lines[center] and the second one in current_line_after.
    wchar_t* current_line_before = lines[center];
    lines[center] = wstrcat(current_line_before, current_line_after, c_linebef_size, c_lineaf_size);
    lines_size[center] = (int)floor((c_linebef_size + c_lineaf_size)/winw);

    // Render everything
    int centerx, centery; // We save centery just in case
    werase(win);
    int wrap_line_offset = 0; //
    for (int i = 0; i < winh; i++) {
        /*if (lines_hasinit[i]) {
            wmove(win, i, 0);
            wprintw(win, "%d", i+1);
        }*/
        if (i == center) {
            wmove(win, i+wrap_line_offset, 0);
            waddwstr(win, current_line_before);
            centerx = getcurx(win);
            centery = getcury(win);
        }
        wmove(win, i+wrap_line_offset, 0);
        waddwstr(win, lines[i]);
        wrap_line_offset += lines_size[i];
    }
    wrefresh(win);

    *cx = centerx;
    *cy = centery;
    wrefresh(stdscr);

    free(lines_hasinit);
    free(lines_size);
    free(current_line_after);
    free(current_line_before);

    if (previous_lines->lines != NULL) {
        for (int i = 0; i < previous_lines->amount; i++) {
            if (previous_lines->lines[i] != NULL) free(previous_lines->lines[i]);
            previous_lines->lines[i] = NULL;
        }
    }
    free(previous_lines->lines);

    previous_lines->amount = winh;
    previous_lines->lines  = lines;
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

void tbuffer_free(Text_buffer* buf) {
    free(buf->after_cursor);
    free(buf->before_cursor);
    free(buf);
}

/*
    BUFFER SYSTEM STUFF
*/

static struct {
    Text_buffer* buffers;
    bool* free;
    u32 current_alloc;
} TB_system;

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
        if (TB_system.free[i]) return i;
    }
    TB_system.buffers = erealloc(TB_system.buffers, sizeof(Text_buffer)*TB_system.current_alloc*2);
    TB_system.free = erealloc(TB_system.free, sizeof(bool)*TB_system.current_alloc*2);
    for (u32 i = TB_system.current_alloc; i < TB_system.current_alloc*2; i++) {
        //tbuffer_init(TB_system.buffers + i, 100);
        TB_system.free[i] = true;
    }

    TB_system.current_alloc *= 2;
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

TBUFID tsFILE_open(const wchar_t* name) {
    FILE* f = fopen(name, "r");
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

    Data_buffer* utf16 = utftrans_8to16(dat->data, dat->cursize);

    if (utf16->cursize % 2 == 1) {
        TB_system_error = TBSE_INVALID_FILE;
        return;
    }
    TBUFID id = ts_ensure_free();
    Text_buffer buf = TB_system.buffers[id];
    buf.b_size = utf16->cursize/2;
    buf.before_cursor = ecalloc(buf.b_size, sizeof(wchar_t));
    buf.after_cursor = ecalloc(buf.b_size, sizeof(wchar_t));
    buf.ac_current_char = buf.b_size;
    buf.bc_current_char = buf.b_size - 1;
    buf.current_chars_stored = buf.b_size - 1;
    buf.flags = TB_WRITTABLE | TB_UPDATED;

    memcpy(buf.before_cursor, utf16->data, sizeof(wchar_t)*(buf.b_size - 1));
    databuffer_free(utf16);
    databuffer_free(dat);
    return id;
}

bool tsFILE_save(TBUFID buf, const char* name) { // TODO: proper error checking
    //FILE* f = fopen(name, "w+");

    return true;
}
void tsFILE_close(TBUFID buf) { // TODO: Get to error checking eventually

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
    b->prevl = ecalloc(1, sizeof(Lines_buffer));
    b->flags = 0;
    return b;
}

void bwindow_handle_keypress(Buffer_window* w, int key) {

    Text_buffer* buf = &TB_system.buffers[w->buf_id];
    bool is_comline = IS_FLAG_ON(buf->flags, TB_COMLINE);
    switch (key) {
    case 13:
    case PADENTER:
        if (is_comline) {
            Wide_string_list* com = command_parse(buf->before_cursor, buf->bc_current_char+1); // TODO: enter in the middle of a command the command gets executed
            Command_response resp = command_execute(com);
            if (resp.resp != COMRESP_OK) {

            }
            free(com);
        }
        else tbuffer_insert(buf, '\n');
        break;

    case 9:
        if (!is_comline) tbuffer_insert(buf, '\t');
        break;

    case 8: // Backspace
        if (buf->bc_current_char != 0) {
            buf->before_cursor[buf->bc_current_char-1] = 0;
            buf->bc_current_char--;
            buf->flags |= TB_UPDATED;
        }
        break;

    case PADSTAR: tbuffer_insert(buf, '*'); break;
    case PADSLASH: tbuffer_insert(buf, '/'); break;
    case PADPLUS: tbuffer_insert(buf, '+'); break;
    case PADMINUS: tbuffer_insert(buf, '-'); break;

    case KEY_LEFT:
        tbuffer_move_cursor(buf, -1);
        break;

    case KEY_RIGHT:
        tbuffer_move_cursor(buf, 1);
        break;

    case KEY_UP: // TODO: If lines only consist of \n some weird things happen
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

    }

    if ((key >= 32 && key <= 0x7E) || (key >= 0xA1 && key <= 0xFF)) {
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
    tbuffer_insert_string_bypass(&TB_system.buffers[w->buf_id], str.str, str.size);
}

void bwindow_update(Buffer_window* w, int* cursorx, int* cursory) {
    Text_buffer* curbuffer = &(TB_system.buffers[w->buf_id]);
    if (IS_FLAG_ON(curbuffer->flags, TB_UPDATED)) {
        SET_FLAG_OFF(curbuffer->flags, TB_UPDATED);
        /* We return the current array of lines from the function to free them after we have already set the new lines.
           This is because addwstr apparently needs the pointer alive and doesn't copy the contents of the string? Idk */
        tbuffer_render(w->curses_window, curbuffer, w->prevl, cursory, cursorx);
    }
}

