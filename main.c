#include <curses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "misc.h"
#include "buffer.h"
#include "databuffer.h"

//#define TEST

/* pdcurses apparently doesn't always free memory? */

WINDOW* makeMenuBar() {
    int w = getmaxx(stdscr);
    WINDOW* ret = newwin(3, w, 0, 0);
    //mvwprintw(ret, 0, 0, " File ");
    wattron(ret, COLOR_PAIR(1));
    mvwprintw(ret, 0, 0, "        ");
    mvwprintw(ret, 1, 0, "  File  ");
    mvwprintw(ret, 2, 0, "        ");
    wattroff(ret, COLOR_PAIR(1));

    return ret;
}

WINDOW* make_textwin() {
    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* textwin = newwin(h-1, w, 0, 0);
    refresh();
    return textwin;
}

int start_display();
bool valid_keypress(int k) {
    // PDCurses really doesn't like foreign keyboards
    return (k >= 32 && k <= 0x7E) ||    // Basic Latin
           (k >= 0xA1 && k <= 0xFF);
           //(k >= 0xA1 && k <= 0xFFEE)
}

int main(int argc, char** argv) {

    Text_buffer* main_buffer = tbuffer_create(2);
    Previous_lines_buffer* previous_lines = ecalloc(1, sizeof(Previous_lines_buffer));

    if (start_display() == -1) return -1;
    WINDOW* textwin = make_textwin();
    int winh, winw;
    getmaxyx(stdscr, winh, winw);

    if (argc > 1) {
        FILE* f = fopen(argv[1], "r");
        Data_buffer* dat = databuffer_new(20);
        int c = 0;
        while(true) {
            c = fgetc(f);
            if (feof(f)) break;
            //addch(c);
            databuffer_add_byte(dat, (unsigned char) c);
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
    }
    refresh();

    bool is_running = true;
    while (is_running) {
        int keypress = getch();

        if (keypress == 27) {
            is_running = false;
        } else if ( keypress == KEY_RESIZE ) {
            resize_term(0, 0);
            free(textwin);
            textwin = make_textwin();
            refresh();
            main_buffer->flags |= 1;
            getmaxyx(stdscr, winh, winw);
        }
#ifdef TEST
        if (keypress != ERR) {
            printw("%d|", keypress);
            refresh();
        }
#endif
#ifndef TEST
        if (keypress == KEY_LEFT) tbuffer_move_cursor(main_buffer, -1);
          else if (keypress == KEY_RIGHT) tbuffer_move_cursor(main_buffer, 1);
          else if (keypress == KEY_UP) { // Scroll up
                int linestart = tbuffer_last_nl_before(main_buffer, main_buffer->bc_current_char);
                if (linestart != 0) {
                    int newlinestart = tbuffer_last_nl_before(main_buffer, linestart - 1);
                    linestart = newlinestart == 0 ? linestart + 1 : linestart;
                    if (linestart - newlinestart < main_buffer->bc_current_char - linestart) {
                        tbuffer_move_cursor(main_buffer, linestart - main_buffer->bc_current_char);
                    } else {
                        tbuffer_move_cursor(main_buffer, newlinestart - linestart);
                    }
                }
          } else if (keypress == KEY_DOWN) { // Scroll down
                // This is code
                int lineend = tbuffer_first_nl_after(main_buffer, main_buffer->ac_current_char) - main_buffer->ac_current_char + main_buffer->bc_current_char;
                // (- main_buffer->ac_current_char + main_buffer->bc_current_char) translates it from after_cursor coord space to before_cursor coord space
                if (lineend == main_buffer->b_size - 1) {
                    tbuffer_move_cursor(main_buffer, main_buffer->b_size - main_buffer->bc_current_char);
                } else {
                    int linestart = tbuffer_last_nl_before(main_buffer, main_buffer->bc_current_char);
                    int newlineend = tbuffer_first_nl_after(main_buffer, lineend - main_buffer->bc_current_char + main_buffer->ac_current_char + 1)
                                     - main_buffer->ac_current_char + main_buffer->bc_current_char;
                    linestart = linestart == 0 ? -1 : linestart;
                    newlineend = newlineend == main_buffer->b_size - 1 ? newlineend - 1 : newlineend;
                    if (main_buffer->bc_current_char - linestart > newlineend - lineend) { // Current line is bigger than following line
                        tbuffer_move_cursor(main_buffer, newlineend - main_buffer->bc_current_char);
                    } else {
                        tbuffer_move_cursor(main_buffer, - linestart + lineend);
                    }
                }
          }
          else if (keypress == 13 || keypress == PADENTER)  tbuffer_insert(main_buffer, '\n');
          else if (keypress == 9) tbuffer_insert(main_buffer, '\t');
          else if (keypress == 8)   { // Backspace
                if (main_buffer->bc_current_char != 0) {
                    main_buffer->before_cursor[main_buffer->bc_current_char-1] = 0;
                    main_buffer->bc_current_char--;
                    main_buffer->flags |= 1;
                }
        } else if (valid_keypress(keypress)) tbuffer_insert(main_buffer, keypress);
          else if (keypress == PADSTAR) tbuffer_insert(main_buffer, '*');
          else if (keypress == PADSLASH) tbuffer_insert(main_buffer, '/');
          else if (keypress == PADPLUS) tbuffer_insert(main_buffer, '+');
          else if (keypress == PADMINUS) tbuffer_insert(main_buffer, '-');

        if ((main_buffer->flags & 1) == 1) {
            main_buffer->flags &= 254;
            mvprintw(winh-1, 0, "                                                                      ");
            mvprintw(winh - 1, 0, "Buffer size:%d bytes|Chars:%d|Line:",
                     main_buffer->b_size*2*2,
                     main_buffer->bc_current_char + main_buffer->b_size - main_buffer->ac_current_char
            );
            refresh();
            /* We return the current array of lines from the function to free them after we have already set the new lines.
               This is because addwstr apparently needs the pointer alive and doesn't copy the contents of the string? Idk */
            tbuffer_render(textwin, main_buffer, previous_lines);
        }
#endif
    }

    endwin();

    // left is 260 and right is 261, up is 259 and down is 258
    return 0;
}

int start_display() {
    initscr();
    noecho();
    raw();
    keypad(stdscr, true);
    wtimeout(stdscr, 1);
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