#include "buffer.h"
#include "misc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Text_buffer* tbuffer_create(int in_size) {
    Text_buffer* res = emalloc(sizeof(Text_buffer));
    res->b_size = in_size;
    res->before_cursor = ecalloc(in_size, sizeof(wchar_t));
    res->after_cursor = ecalloc(in_size, sizeof(wchar_t));
    res->ac_current_char = in_size;
    res->bc_current_char = 0;
    res->current_chars_stored = 0;
    res->flags = 0;
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
    res->flags = 1;

    memcpy(res->before_cursor, dat->data, sizeof(wchar_t)*(res->b_size - 1));
    return res;
}

void tbuffer_insert(Text_buffer* buf, wchar_t c) {
    if (buf->current_chars_stored >= buf->b_size) {
        tbuffer_resize(buf);
    }
    buf->before_cursor[buf->bc_current_char] = c;
    buf->current_chars_stored++;
    buf->bc_current_char++;

    buf->flags |= 1;
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
    memcpy(new_after + old_size, buf->after_cursor, sizeof(wchar_t)*old_size);
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

        buf->flags |= 1;
        return actual;
    } else if (amount > 0) {
        int actual = amount;
        if (amount+buf->ac_current_char > buf->b_size) actual = buf->b_size - buf->ac_current_char;
        memcpy(buf->before_cursor + buf->bc_current_char, buf->after_cursor + buf->ac_current_char, sizeof(wchar_t)*actual);
        memset(buf->after_cursor + buf->ac_current_char, 0, sizeof(wchar_t)*actual);

        buf->bc_current_char += actual;
        buf->ac_current_char += actual; // plus bcs it's flipped.

        buf->flags |= 1;
        return actual;
    }
    return 0;
}

/* We start iterating over chars that are near the cursor, so we find line breaks and
   get the amount of chars between them so we can render lines correctly */
void tbuffer_render(WINDOW* win, Text_buffer* buf, Previous_lines_buffer* previous_lines) {

    int winh = getmaxy(win);
    int center = winh % 2 == 0 ? winh/2 : (winh-1)/2;
    int line_offset = 0;
    wchar_t** lines = ecalloc(winh, sizeof(wchar_t*));
    bool*     lines_hasinit = ecalloc(winh, sizeof(bool));
    int c_linebef_size = 0;
    int start_offset = 0;

    // before_cursor and first part of the current line
    for (int i = 0; i <= buf->bc_current_char; i++) {
        if (buf->before_cursor[buf->bc_current_char - i] == '\n' ) {
            wchar_t* str = tbuffer_translate_string(buf, BO_BEFORE, buf->bc_current_char - i + 1, buf->bc_current_char - start_offset);
            lines[center+line_offset] = str;
            lines_hasinit[center+line_offset] = true;

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
            } else {
                current_line_after = ecalloc(i - start_offset, sizeof(wchar_t));
                memcpy(current_line_after, buf->after_cursor + buf->ac_current_char + start_offset, (i - start_offset)*sizeof(wchar_t));
                c_lineaf_size = i - start_offset;
            }

            start_offset = i+1;
            line_offset++;
        } else if (i == buf->b_size - buf->ac_current_char) { // The last line needs special treatment bcs it's not delimited with \n
            if (line_offset != 0) { // Again, we handle this later
                wchar_t* str = tbuffer_translate_string(buf, BO_AFTER, buf->ac_current_char + start_offset, buf->ac_current_char + i + 1);
                lines[center+line_offset] = str;
                lines_hasinit[center+line_offset] = true;
            } else {
                current_line_after = ecalloc(i - start_offset, sizeof(wchar_t));
                memcpy(current_line_after, buf->after_cursor + buf->ac_current_char + start_offset, (i - start_offset)*sizeof(wchar_t)); //ok
                c_lineaf_size = i - start_offset;
            }

            start_offset = i+1;
            line_offset++;
        }
        if (center+line_offset > winh-1) break;
    }

    // Correctly get the current line. We have the first part in lines[center] and the second one in current_line_after.
    wchar_t* current_line_before = lines[center];
    lines[center] = wstrcat(current_line_before, current_line_after, c_linebef_size, c_lineaf_size);

    // Render everything
    int centerx, centery; // We save centery just in case
    werase(win);
    for (int i = 0; i < winh; i++) {
        if (lines_hasinit[i]) {
            wmove(win, i, 0);
            wprintw(win, "%d", i+1);
        }
        if (i == center) {
            wmove(win, i, 8); // We have to put this so far from the start because curses starts tabbing at the start of the console window
            waddwstr(win, current_line_before);
            centerx = getcurx(win);
            centery = getcury(win);
        }
        wmove(win, i, 8);
        waddwstr(win, lines[i]);
    }
    wrefresh(win);

    move(centery, centerx);
    wrefresh(stdscr);

    free(lines_hasinit);
    free(current_line_after);
    free(current_line_before);

    // We free the lines that were on before. We do it now instead of before so the program doesn't crash
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
    u8* free;
} Textbuffer_system;


