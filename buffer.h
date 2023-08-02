#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <curses.h>
#include "databuffer.h"

/* Text_buffer stores text in 2 different buffers: one that consists of text before the cursor
and one that consists of text after the cursor. This is pretty handy for modifying lots of characters around
a position (for example, the cursor) Fields that talk about chars are about the unicode
UTF-16 type (2 bytes) instead of the standard 1 byte. Windows' default console font doesn't even have unicode
characters over FFFF so it's probably not worth it to have full unicode support bcs I'm probably not gonna
use it.
A text like abcd|efg where | is the cursor may be stored in these buffers like:
                    p
before_cursor = abcd···
after_cursor  = ····efg
                   p*/
typedef struct {
    wchar_t* before_cursor;
    wchar_t* after_cursor;
    int b_size;
    int bc_current_char;
    int ac_current_char;
    int current_chars_stored;
    char flags; // 1's bit is if it has updated
} Text_buffer;

/* Used to store the lines that were on the previous frame to be able to free them when the screen changes */
typedef struct {
    wchar_t** lines;
    int amount;
} Previous_lines_buffer;

typedef enum {
    BO_BEFORE,
    BO_AFTER
} Buffer_order;

/* Initializers */
Text_buffer* tbuffer_create(int in_size);
Text_buffer* tbuffer_from_databuffer(Data_buffer* dat); /* Creates a Text_buffer with before_cursor from dat->data */

void tbuffer_insert(Text_buffer* buf, wchar_t c); /* Types a character where the cursor is */
void tbuffer_resize(Text_buffer* buf); /* Doubles the storage available in the buffer */
void tbuffer_render(WINDOW* win, Text_buffer* buf, Previous_lines_buffer* previous_lines); /* Renders the lines that are visible on the given window */

int tbuffer_move_cursor(Text_buffer* buf, int amount); /* Moves the cursor amount characters backwards(negative amount) or forwards (positive amount)*/
int tbuffer_last_nl_before(Text_buffer* buf, int pos); /* Finds the last newline before the given position */
int tbuffer_first_nl_after(Text_buffer* buf, int pos); /* Finds the first newline after the given position */

wchar_t* tbuffer_translate_string(Text_buffer* buf, Buffer_order b, int st, int en); /* Translates a section of the given buffer into a wide string */

#ifdef DEBUG
void tbuffer_read(Text_buffer* buf);
#endif

#endif // BUFFER_H_INCLUDED
