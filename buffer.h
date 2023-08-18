#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <curses.h>
#include <stdlib.h>
#include <stdarg.h>

#include "types.h"
#include "wstr.h"
#include "databuffer.h"
#include "strlist.h"

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
typedef struct { // TODO: Make this into an actual "gap buffer" (basically the optimized version of this)
    wchar_t* before_cursor;
    wchar_t* after_cursor;
    int b_size;
    int bc_current_char; // We are not supporting huge files
    int ac_current_char;
    int current_chars_stored;
    Narrow_string linked_file_path;
    u8 flags;
} Text_buffer;

/* Used to store the lines that were on the previous frame to be able to free them when the screen changes */
typedef struct {
    wchar_t** lines;
    int amount;
} Lines_buffer;

/* Some functions require to know if you want to do something to the before_cursor buffer or the after_cursor buffer */
typedef enum {
    BO_BEFORE,
    BO_AFTER
} Buffer_order;

typedef enum {
    TB_UPDATED = 1, // Has the text buffer been updated
    TB_WRITTABLE = 2, // Is the text buffer writtable
    TB_ENDSCROLL = 4, // Should the cursor be in the end of the buffer window or in the middle
    TB_COMLINE = 8, // Should the text buffer act like the command feedback thingy
    TB_COMOUTPUT = 16 // Is the text buffer the input for commands
} Text_buffer_flags;

struct {
    Text_buffer* buffers;
    bool* free;
    u32 current_alloc;
} TB_system; // Do not do weird memory stuff with this outside of ts functions

typedef enum {
    TBSE_OK,
    TBSE_ALREADY_FREE,
    TBSE_OUT_OF_BOUNDS,
    TBSE_FILE_NOT_FOUND,
    TBSE_INVALID_FILE,
    TBSE_INVALID_PATH,
    TBSE_CANT_CREATE_FILE
} TB_system_error_code;

TB_system_error_code TB_system_error;

typedef u32 TBUFID;

typedef struct {
    WINDOW* curses_window;
    Lines_buffer* curl;
    Lines_buffer* prevl;
    TBUFID buf_id;
    int win_id;
    u8 flags; // 1's bit visible
} Buffer_window;

typedef enum {
    BW_VISIBLE = 1,
    BW_FILE = 2 // Is this buffer window the one for files?
} Buffer_window_flags;

/* Initializers */
Text_buffer* tbuffer_create(int in_size);
void         tbuffer_init(Text_buffer* buf, int in_size);
Text_buffer* tbuffer_from_databuffer(Data_buffer* dat); /* Creates a Text_buffer with before_cursor from dat->data */

/* Modifies the buffer */
bool tbuffer_insert(Text_buffer* buf, wchar_t c); /* Types a character where the cursor is */
void tbuffer_insert_bypass(Text_buffer* buf, wchar_t c); /* Ignores the TB_WRITTABLE tag */
void tbuffer_insert_string_bypass(Text_buffer* buf, wchar_t* str, int sz); /* Bypasses writtable tag */
void tbuffer_insert_formatted_bypass(Text_buffer* buf, const wchar_t* formatstring, ...);
bool tbuffer_backspace(Text_buffer* buf); /* Backspaces in the current cursor position */
void tbuffer_clear(Text_buffer* buf); /* Sets the whole buffer to 0s */

int tbuffer_move_cursor(Text_buffer* buf, int amount); /* Moves the cursor amount characters backwards(negative amount) or forwards (positive amount)*/
void tbuffer_move_cursor_to_pos(Text_buffer* buf, int pos); /* Moves the cursor to the given pos in the text buffer */

/* Necessary memory management */
void tbuffer_resize(Text_buffer* buf); /* Doubles the storage available in the buffer */
void tbuffer_resize_custom(Text_buffer* buf, int sz); /* Enlarges the buffer by sz */
void tbuffer_free(Text_buffer* buf);

/* Search functions */
int tbuffer_last_nl_before(Text_buffer* buf, int pos); /* Finds the last newline before the given position */
int tbuffer_first_nl_after(Text_buffer* buf, int pos); /* Finds the first newline after the given position */
wchar_t* tbuffer_translate_string(Text_buffer* buf, Buffer_order b, int st, int en); /* Translates a section of the given buffer into a wide string */
int tbuffer_find_line(Text_buffer* buf, int l); /* Finds the position of the start of the l'th line*/
int tbuffer_get_cursor_line(Text_buffer* buf); /* Finds the line the cursor is currently in */

void tbuffer_render(Buffer_window* bwin, Text_buffer* buf, Lines_buffer* previous_lines, int* cy, int* cx); /* Renders the lines that are visible on the given window */

/*
    Buffer system functions
    Allows us to handle multiple files being opened at once, handles all memory stuff of that
    so we don't have 100 mallocs in different places
*/

void ts_start();
void ts_shutdown();
void ts_free_buffer(TBUFID id);
TBUFID ts_ensure_free();
TBUFID tsFILE_new();
TBUFID tsFILE_open(const u8* name, u32 name_size);
void tsFILE_save(TBUFID buf);

/*
    Buffer window functions
*/

Buffer_window* bwindow_create();
void bwindow_assign_tbuffer(Buffer_window* w, Text_buffer* b);
void bwindow_handle_keypress(Buffer_window* w, int keypress, u64 key_mods);
void bwindow_buf_set_flags_on(Buffer_window* w, u8 flags);
void bwindow_buf_set_flags_off(Buffer_window* w, u8 flags);
void bwindow_buf_insert_text(Buffer_window* w, Wide_string str);
void bwindow_update(Buffer_window* w, int winh, int* cursorx, int* cursory, bool is_selected_window);

/*
    File buffer window functions
*/

void fbw_start(TBUFID fb);
void fbw_shutdown();
u32 fbw_add_entry(TBUFID newbufid, const wchar_t* path, u32 pathsz, int* winid); /* Adds an entry into the file buffer window */
int fbw_find_entry(TBUFID id); /* Finds the index to the entry that has the given ID. Returns -1 if it isn't found */
void fbw_mark_bind(TBUFID id, int win_id);
void fbw_mark_unbind(TBUFID id);

#endif // BUFFER_H_INCLUDED
