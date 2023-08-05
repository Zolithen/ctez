#include <curses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#include "misc.h"
#include "buffer.h"
#include "databuffer.h"

#include <windows.h> // Used for platform_sleep
#define BOTTOM_SECTION_HEIGHT 12

// TODO: It's probably much better to make something like a system for handling windows
WINDOW* make_textwin(int h, int w) {
    WINDOW* textwin = newwin(h-2, w, 0, 0);
    refresh();
    return textwin;
}

WINDOW* make_commandwin(int h, int w) {
    WINDOW* commandwin = newwin(1, w, h-1, 0);
    refresh();
    return commandwin;
}

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

void show_monolithic_layout(Buffer_window** wins, int winh, int winw) {
    for (int i = 3; i < 6; i++) {
        SET_FLAG_OFF(wins[i]->flags, BW_VISIBLE);
    }
    wins[2]->flags |= BW_VISIBLE;
    delwin(wins[2]->curses_window);

    wins[2]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw, 0, 0);
    wins[2]->current_buffer->flags |= TB_UPDATED;
}

void show_double_layout(Buffer_window** wins, int winh, int winw) {
    for (int i = 4; i < 6; i++) {
        SET_FLAG_OFF(wins[i]->flags, BW_VISIBLE);
    }
    wins[2]->flags |= BW_VISIBLE;
    delwin(wins[2]->curses_window);
    wins[3]->flags |= BW_VISIBLE;
    delwin(wins[3]->curses_window);

    if (winw % 2 == 0) {
        wins[2]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw/2 - 1, 0, 0);
        wins[2]->current_buffer->flags |= TB_UPDATED;
        wins[3]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, winw/2 - 1, 0, winw/2 + 1);
        wins[3]->current_buffer->flags |= TB_UPDATED;
    } else {
        wins[2]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, (int)floor(winw/2), 0, 0);
        wins[2]->current_buffer->flags |= TB_UPDATED;
        wins[3]->curses_window = newwin(winh-BOTTOM_SECTION_HEIGHT, (int)floor(winw/2), 0, floor(winw/2) + 1);
        wins[3]->current_buffer->flags |= TB_UPDATED;
    }
}

int main(int argc, char** argv) {

    // Start up curses
    if (start_display() == -1) return -1;
    int winh, winw;
    getmaxyx(stdscr, winh, winw);
    int cursorx, cursory;
    int selwinx, selwiny;

    // Structs
    Buffer_window** bwindows = ecalloc(6, sizeof(Buffer_window*));

    // Setup layouts
    int editor_layout = 0; // 0 is monolithic, 1 is double & 2 is four

    for (int i = 2; i < 6; i++) {
        bwindows[i] = bwindow_create();
        bwindows[i]->curses_window = newwin(1, 1, 0, 0);
    }

    bwindows[0] = bwindow_create();
    bwindows[1] = bwindow_create();

    show_double_layout(bwindows, winh, winw);

    int selected_window = 3;
    getbegyx(bwindows[selected_window]->curses_window, selwiny, selwinx);

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
        Text_buffer* curbuffer = curwin->current_buffer;
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
            if (selected_window == 2) selected_window = 3;
            else selected_window = 2;
            getbegyx(bwindows[selected_window]->curses_window, selwiny, selwinx);
            bwindows[selected_window]->current_buffer->flags |= TB_UPDATED;
        }

        //mvprintw(winh-1, 0, "");
        /*mvprintw(winh - 2, 0, "Buffer size:%d bytes|Chars:%d|Line:",
            curbuffer->b_size*2*2,
            curbuffer->bc_current_char + curbuffer->b_size - curbuffer->ac_current_char
        );*/

        bwindow_handle_keypress(curwin, keypress);
        for (int i = 0; i < 6; i++) {
            curwin = bwindows[i];
            curbuffer = curwin->current_buffer;
            if (IS_FLAG_ON(curbuffer->flags, TB_UPDATED)) {
                SET_FLAG_OFF(curbuffer->flags, TB_UPDATED);
                /* We return the current array of lines from the function to free them after we have already set the new lines.
                    This is because addwstr apparently needs the pointer alive and doesn't copy the contents of the string? Idk */
                tbuffer_render(curwin->curses_window, curbuffer, curwin->prevl, &cursory, &cursorx);
            }
        }

        move(selwiny + cursory, selwinx + cursorx);
        refresh();
    }

    endwin();
    return 0;
}
