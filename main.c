#include <curses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#include "misc.h"
#include "buffer.h"
#include "databuffer.h"
#include "types.h"
#include "strlist.h"
#include "command.h"

#include <windows.h> // Used for platform_sleep
#define BOTTOM_SECTION_HEIGHT 12


void platform_sleep(int milisecs) { // You can change this one function to the platform's sleep function (it's not in the C standard)
    Sleep(milisecs); // Windows
}

int start_display() {
    initscr();
    noecho();
    raw();
    keypad(stdscr, true);
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

    if (winw % 2 == 0) {
        wins[TWINCOM]->curses_window = newwin(BOTTOM_SECTION_HEIGHT - 1, winw/2 - 1, winh - BOTTOM_SECTION_HEIGHT + 1, 0);
    } else {
        wins[TWINCOM]->curses_window = newwin(BOTTOM_SECTION_HEIGHT - 1, (int)floor(winw/2), winh - BOTTOM_SECTION_HEIGHT + 1, 0);
    }

    bwindow_buf_set_flags_on(wins[TWINCOM], TB_UPDATED | TB_ENDSCROLL | TB_COMLINE);
}

void show_monolithic_layout(Buffer_window** wins, int winh, int winw) {
    erase();
    for (int i = TWIN2; i < MAX_WINDOWS; i++) {
        SET_FLAG_OFF(wins[i]->flags, BW_VISIBLE);
    }
    wins[TWIN1]->flags |= BW_VISIBLE;
    delwin(wins[TWIN1]->curses_window);

    wins[TWIN1]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw, 0, 0);
    bwindow_buf_set_flags_on(wins[TWIN1], TB_UPDATED);

    show_bottom_of_screen(wins, winh, winw);
}

void show_double_layout(Buffer_window** wins, int winh, int winw) {
    erase();
    for (int i = TWIN3; i < MAX_WINDOWS; i++) {
        SET_FLAG_OFF(wins[i]->flags, BW_VISIBLE);
    }
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

int main(int argc, char** argv) {

    // Start up curses
    if (start_display() == -1) return -1;
    int winh, winw;
    getmaxyx(stdscr, winh, winw);
    int cursorx, cursory;
    int selwinx, selwiny;

    ts_start();

    // Structs
    Buffer_window** bwindows = ecalloc(MAX_WINDOWS, sizeof(Buffer_window*));

    // Setup layouts
    int editor_layout = 0; // 0 is monolithic, 1 is double & 2 is four

    for (int i = TWIN1; i < MAX_WINDOWS; i++) {
        bwindows[i] = bwindow_create();
        bwindows[i]->curses_window = newwin(1, 1, 0, 0);
    }

    bwindows[TWINCOM] = bwindow_create();
    bwindows[TWINFILE] = bwindow_create();

    show_double_layout(bwindows, winh, winw);

    int selected_window = TWIN2;
    getbegyx(bwindows[selected_window]->curses_window, selwiny, selwinx);

    /*Wide_string_list* test = command_parse(L"open \"D:/c/proj/ctez\"", 22); // with terminator
    wstrlist_debug_print(test);
    wstrlist_free(test);*/

    /*wchar_t* t = ecalloc(3, sizeof(wchar_t));
    t[0] = 'h';
    t[1] = 'o';
    t[2] = 'l';
    Wide_string_list* test = wstrlist_new(4);
    wstrlist_add_and_terminator(test, t, 3);
    free(t);
    t = ecalloc(9, sizeof(wchar_t));
    t[0] = 't';
    t[1] = 'e';
    t[2] = 's';
    t[3] = 't';
    t[4] = ' ';
    t[5] = 's';
    t[6] = 't';
    t[7] = 'r';
    wstrlist_add(test, t, 9);
    free(t);
    t = ecalloc(4, sizeof(wchar_t));
    t[0] = 't';
    t[1] = 'o';
    t[2] = 'n';
    t[3] = 't';
    wstrlist_add_and_terminator(test, t, 4);
    Wide_string w1 = wstrlist_get(test, 0);
    Wide_string w2 = wstrlist_get(test, 1);
    Wide_string w3 = wstrlist_get(test, 2);
    free(t);
    wstrlist_debug_print(test);
    wstrlist_free(test);*/



    /*if (argc > 1) {
        FILE* f = fopen(argv[1], "r");
        Data_buffer* dat = databuffer_new(20);
        int c = 0;
        while(true) {
            c = fgetc(f);
            if (feof(f)) break;
            databuffer_add_byte(dat, (u8) c);
        }
        databuffer_add_byte(dat, '\0');

        Data_buffer* utf16 = utftrans_8to16(dat->data, dat->cursize);
        move(6, 0);

        // TODO: error checking
        free(main_buffer);
        main_buffer = tbuffer_from_databuffer(utf16);
        fclose(f);
        databuffer_free(dat);
        databuffer_free(utf16);
    }*/
    refresh();
    bool is_running = true;
    while (is_running) {
        platform_sleep(1);

        Buffer_window* curwin = bwindows[selected_window];
        //Text_buffer* curbuffer = curwin->current_buffer;
        int keypress = getch();

        if (keypress == 27) {
            is_running = false;
        } else if ( keypress == KEY_RESIZE ) {
            resize_term(0, 0);
            getmaxyx(stdscr, winh, winw);

            show_double_layout(bwindows, winh, winw);
            getbegyx(bwindows[selected_window]->curses_window, selwiny, selwinx);

            refresh();
        } else if (keypress == KEY_F(1)) {
            if (selected_window == TWIN2) selected_window = TWIN1;
            else selected_window = TWIN2;
            getbegyx(bwindows[selected_window]->curses_window, selwiny, selwinx);
            bwindow_buf_set_flags_on(bwindows[selected_window], TB_UPDATED);
        }

        //mvprintw(winh-1, 0, "");
        /*mvprintw(winh - 2, 0, "Buffer size:%d bytes|Chars:%d|Line:",
            curbuffer->b_size*2*2,
            curbuffer->bc_current_char + curbuffer->b_size - curbuffer->ac_current_char
        );*/

        bwindow_handle_keypress(curwin, keypress);
        for (int i = 0; i < MAX_WINDOWS; i++) {
            bwindow_update(bwindows[i], &cursorx, &cursory);
        }

        move(selwiny + cursory, selwinx + cursorx);
        refresh();
    }

    endwin();
    return 0;
}
