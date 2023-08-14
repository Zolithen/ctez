#include <curses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#include "app.h"
#include "types.h"
#include "misc.h"
#include "wstr.h"
#include "list.h"
#include "databuffer.h"
#include "strlist.h"

#include "buffer.h"
#include "command.h"

#include <windows.h> // Used for platform_sleep

#define BOTTOM_SECTION_HEIGHT 12

void platform_sleep(int milisecs) { // You can change this one function to the platform's sleep function (it's not in the C standard)
    Sleep(milisecs); // Windows
}

// TODO: Add printf variant
void print_twincom(Wide_string msg) {
    bwindow_buf_insert_text(App.bwindows[TWINCOM], msg);
}

void printf_twincom(const wchar_t* formatstring, ...) {
    va_list args;
    va_start(args, formatstring);

    Wide_string msg = { 0 };
    int chars_to_alloc = _vscwprintf(formatstring, args) + 1; // _vscwprintf doesn't count the terminating char
    msg.size = chars_to_alloc;
    msg.str = emalloc(chars_to_alloc * sizeof(wchar_t));
    _vsnwprintf_s(msg.str, chars_to_alloc, chars_to_alloc - 1, formatstring, args);
    tbuffer_insert_string_bypass(&TB_system.buffers[App.bwindows[TWINCOM]->buf_id], msg.str, msg.size);

    free(msg.str);
    va_end(args);
}

int start_display() {
    initscr();
    noecho();
    raw();
    keypad(stdscr, true);
    //PDC_return_key_modifiers(true);
    //wtimeout(stdscr, 1); // May be needed in other platforms
    wtimeout(stdscr, 0);
    curs_set(2);

    if (!has_colors()) {
        printw("Terminal doesn't support colors");
        getch();
        return -1;
    }
    start_color();

    init_pair(1, COLOR_WHITE, COLOR_CYAN);
    move(0, 0);

    return 0;
}

void show_bottom_of_screen(Buffer_window** wins, int winh, int winw) {
    wins[TWINCOM]->flags |= BW_VISIBLE;
    delwin(wins[TWINCOM]->curses_window);
    wins[TWINFILE]->flags |= BW_VISIBLE;
    delwin(wins[TWINFILE]->curses_window);
    wins[TWINCOMINPUT]->flags |= BW_VISIBLE;
    delwin(wins[TWINCOMINPUT]->curses_window);

    if (winw % 2 == 0) {
        wins[TWINCOM]->curses_window = newwin(BOTTOM_SECTION_HEIGHT - 3, winw/2 - 1, winh - BOTTOM_SECTION_HEIGHT + 1, 0);
        wins[TWINFILE]->curses_window = newwin(BOTTOM_SECTION_HEIGHT - 1, winw/2 - 1, winh - BOTTOM_SECTION_HEIGHT + 1, winw/2 + 1);
        wins[TWINCOMINPUT]->curses_window = newwin(1, winw/2 - 1, winh - 1, 0);
    } else {
        wins[TWINCOM]->curses_window = newwin(BOTTOM_SECTION_HEIGHT - 3, (int)floor(winw/2), winh - BOTTOM_SECTION_HEIGHT + 1, 0);
        wins[TWINFILE]->curses_window = newwin(BOTTOM_SECTION_HEIGHT - 1, (int)floor(winw/2), winh - BOTTOM_SECTION_HEIGHT + 1, floor(winw/2) + 1);
        wins[TWINCOMINPUT]->curses_window = newwin(1, (int)floor(winw/2), winh - 1, 0);
    }

    bwindow_buf_set_flags_on(wins[TWINCOM], TB_UPDATED | TB_ENDSCROLL | TB_COMOUTPUT);
    bwindow_buf_set_flags_off(wins[TWINCOM], TB_WRITTABLE);

    bwindow_buf_set_flags_on(wins[TWINFILE], TB_UPDATED | TB_ENDSCROLL);
    bwindow_buf_set_flags_off(wins[TWINFILE], TB_WRITTABLE);

    bwindow_buf_set_flags_on(wins[TWINCOMINPUT], TB_COMLINE);

    for (int i = 0; i < winw; i++) {
        mvaddch(winh - BOTTOM_SECTION_HEIGHT, i, 0x2501);
        if (i < winw/2) mvaddch(winh - 2, i, 0x2501);
    }
}

void show_monolithic_layout(Buffer_window** wins, int winh, int winw) {
    erase();
    SET_FLAG_OFF(wins[TWIN2]->flags, BW_VISIBLE);
    SET_FLAG_OFF(wins[TWIN3]->flags, BW_VISIBLE);
    SET_FLAG_OFF(wins[TWIN4]->flags, BW_VISIBLE);

    wins[TWIN1]->flags |= BW_VISIBLE;
    delwin(wins[TWIN1]->curses_window);

    wins[TWIN1]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw, 0, 0);
    bwindow_buf_set_flags_on(wins[TWIN1], TB_UPDATED);

    show_bottom_of_screen(wins, winh, winw);

    for (int i = winh-BOTTOM_SECTION_HEIGHT; i < winh; i++) {
        if (winw % 2 == 0) {
            mvaddch(i, winw/2 - 1, 0x2503);
            mvaddch(i, winw/2, 0x2503);
        } else {
            mvaddch(i, (int)floor(winw/2), 0x2503);
        }
    }
}

void show_double_layout(Buffer_window** wins, int winh, int winw) {
    erase();

    SET_FLAG_OFF(wins[TWIN3]->flags, BW_VISIBLE);
    SET_FLAG_OFF(wins[TWIN3]->flags, BW_VISIBLE);

    wins[TWIN1]->flags |= BW_VISIBLE;
    delwin(wins[TWIN1]->curses_window);
    wins[TWIN2]->flags |= BW_VISIBLE;
    delwin(wins[TWIN2]->curses_window);

    if (winw % 2 == 0) {
        wins[TWIN1]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw/2 - 1, 0, 0);
        wins[TWIN2]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw/2 - 1, 0, winw/2 + 1);
    } else {
        wins[TWIN1]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, (int)floor(winw/2), 0, 0);
        wins[TWIN2]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, (int)floor(winw/2), 0, floor(winw/2) + 1);
    }

    bwindow_buf_set_flags_on(wins[TWIN1], TB_UPDATED);
    bwindow_buf_set_flags_on(wins[TWIN2], TB_UPDATED);

    show_bottom_of_screen(wins, winh, winw);

    for (int i = 0; i < winh; i++) {
        if (winw % 2 == 0) {
            mvaddch(i, winw/2 - 1, 0x2503);
            mvaddch(i, winw/2, 0x2503);
        } else {
            mvaddch(i, (int)floor(winw/2), 0x2503);
        }
    }
    refresh();
}

void show_layout(int winh, int winw) {
    if (App.layout == 0) {
        show_monolithic_layout(App.bwindows, winh, winw);
    } else if (App.layout == 1) {
        show_double_layout(App.bwindows, winh, winw);
    } else if (App.layout == 2) {

    }
}

int main() {

    int_to_wstr_char_array[0] = L'0';
    int_to_wstr_char_array[1] = L'1';
    int_to_wstr_char_array[2] = L'2';
    int_to_wstr_char_array[3] = L'3';
    int_to_wstr_char_array[4] = L'4';
    int_to_wstr_char_array[5] = L'5';
    int_to_wstr_char_array[6] = L'6';
    int_to_wstr_char_array[7] = L'7';
    int_to_wstr_char_array[8] = L'8';
    int_to_wstr_char_array[9] = L'9';

    // Start up curses
    if (start_display() == -1) return -1;
    int winh, winw;
    int cursorx, cursory;
    int selwinx, selwiny;
    getmaxyx(stdscr, winh, winw);

    wstr_start();
    ts_start();
    TB_system_error = TBSE_OK;

    // Structs
    App.bwindows = ecalloc(MAX_WINDOWS, sizeof(Buffer_window*));

    // Setup layouts
    App.layout = 1; // 0 is monolithic, 1 is double & 2 is four

    App.bwindows[TWINCOM] = bwindow_create();
    App.bwindows[TWINFILE] = bwindow_create();
    App.bwindows[TWINCOMINPUT] = bwindow_create();

    App.bwindows[TWINCOM]->win_id = TWINCOM;
    App.bwindows[TWINFILE]->win_id = TWINFILE;
    App.bwindows[TWINCOMINPUT]->win_id = TWINCOMINPUT;

    fbw_start(App.bwindows[TWINFILE]->buf_id);

    for (int i = MAX_WINDOWS - 1; i >= TWIN4; i--) {
        Buffer_window* win = bwindow_create();
        win->curses_window = newwin(1, 1, 0, 0);
        win->win_id = i;
        fbw_add_entry(win->buf_id, STR_SYS_NEW_BUFFER.str, STR_SYS_NEW_BUFFER.size, &i);
        fbw_mark_bind(win->buf_id, i);
        App.bwindows[i] = win;
    }

    show_layout(winh, winw);

    App.selwin = TWIN1;
    App.last_selwin = TWIN1;
    getbegyx(App.bwindows[App.selwin]->curses_window, selwiny, selwinx);

    /*Wide_string_list* test = command_parse(L"open \"D:/c/proj/ctez\"", 22); // with terminator
    wstrlist_debug_print(test);
    wstrlist_free(test);*/

    refresh();
    App.is_running = true;
    while (App.is_running) {
        platform_sleep(5);

        Buffer_window* curwin = App.bwindows[App.selwin];
        int keypress = getch();
        /*if (keypress != -1) {
            printf_twincom(L"key %d,\n", keypress);
        }*/
        u64 key_mods = PDC_get_key_modifiers();
        /*if (keypress != -1) {
            printf("%d\n", keypress);
        }*/

        if (keypress == 27) { // We will eventually change this to a command
            App.is_running = false;
        } else if ( keypress == KEY_RESIZE ) { // TODO: there's stil a memory leak? (if there is, it's minor)
            resize_term(0, 0);
            getmaxyx(stdscr, winh, winw);

            show_layout(winh, winw);
            getbegyx(curwin->curses_window, selwiny, selwinx);

            refresh();
        } else if (keypress == KEY_F(1)) {
            App.selwin++;
            if (App.selwin >= MAX_WINDOWS) App.selwin = 0;
            if (App.layout == 0) {
                if ((App.selwin == TWIN2)
                 || (App.selwin == TWIN3)
                 || (App.selwin == TWIN4)) App.selwin = TWIN1;
            } else if (App.layout == 1) {
                if ((App.selwin == TWIN3)
                 || (App.selwin == TWIN4)) App.selwin = TWIN2;
            }
            curwin = App.bwindows[App.selwin];
            getbegyx(curwin->curses_window, selwiny, selwinx);
            bwindow_buf_set_flags_on(curwin, TB_UPDATED);
        } else if (keypress == KEY_F(2)) {
            /*printf("%d\n", fbw_find_entry(curwin->buf_id));
            Text_buffer* tbuf = &TB_system.buffers[curwin->buf_id];
            int pos = tbuffer_find_line(tbuf, 3);
            if (pos != -1) {
                tbuffer_move_cursor_to_pos(tbuf, pos);
            }*/
            //fbw_mark_bound(App.bwindows[TWIN3]);
        } else if (keypress == KEY_F(3)) {

            tbuffer_insert_formatted_bypass(&TB_system.buffers[curwin->buf_id], L"%d HOLA %d %d TEST ", 3, 4, 5);
        }

        //mvprintw(winh-1, 0, "");
        /*mvprintw(winh - 2, 0, "Buffer size:%d bytes|Chars:%d|Line:",
            curbuffer->b_size*2*2,
            curbuffer->bc_current_char + curbuffer->b_size - curbuffer->ac_current_char
        );*/

        bwindow_handle_keypress(curwin, keypress, key_mods);
        for (int i = 0; i < MAX_WINDOWS; i++) {
            // TODO: Do not update windows that are outside the selected_layout
            if (App.selwin == i) {
                bwindow_update(App.bwindows[i], winh, &cursorx, &cursory, true);
            } else {
                bwindow_update(App.bwindows[i], winh, NULL, NULL, false);
            }
        }

        //if (cursory == 0) cursory = ;
        move(selwiny + cursory, selwinx + cursorx);
        refresh();
    }

    ts_shutdown();
    fbw_shutdown();
    endwin();
    return 0;
}
