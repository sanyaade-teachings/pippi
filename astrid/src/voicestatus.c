#include "astrid.h"
#include <ncurses.h>

#define VOICE_NAME_LEN 20
#define ENTER_KEY 10

/* 1000, 800, 176 */

// Colors
#define THEME_PLAYING 1
#define THEME_LOOPING 2
#define THEME_HIGHLIGHT 3


int selected_row_index, x, y;

int main() {
    sqlite3 * sessiondb;
    sqlite3_stmt * stmt;
    int width, height, i;

    selected_row_index = 0;

    lpsessiondb_open_for_reading(&sessiondb);

    /* Init the curses environment */
    initscr();
    noecho();
    keypad(stdscr, TRUE);  // Enable arrow keys
    nodelay(stdscr, TRUE); // Return input immediately
    curs_set(0);  // Hide cursor

    /* Set up colors */
    start_color();
    init_color(COLOR_YELLOW, 1000, 800, 176);
    init_pair(THEME_PLAYING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(THEME_LOOPING, COLOR_WHITE, COLOR_BLACK);
    init_pair(THEME_HIGHLIGHT, COLOR_BLACK, COLOR_YELLOW);

    if(sqlite3_prepare_v2(sessiondb, "select * from voices where active=1;", -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Problem preparing select statement: %s\n", sqlite3_errmsg(sessiondb));
        lpsessiondb_close(sessiondb);
        return 1;
    }

    while(1) {
        x = 0;
        y = 0;

        getmaxyx(stdscr, height, width);

        attron(A_BOLD);
        attron(COLOR_PAIR(THEME_PLAYING));
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            mvprintw(y, x, "%3d: %-20s %d", 
                sqlite3_column_int(stmt, 6), 
                sqlite3_column_text(stmt, 7), 
                sqlite3_column_int(stmt, 9)
            );
            y += 1;
        }
        attroff(COLOR_PAIR(THEME_PLAYING));
        attroff(A_BOLD);

        sqlite3_reset(stmt);

        for(i=y; i < height; i++) {
            move(i, 0);
            clrtoeol();
        }

        mvprintw(height - 1, x, "Press 'q' to quit. Press ENTER to select an item.");

        // Handle input
        int ch = getch();
        switch(ch) {
            case KEY_UP:
                //ctx.selected_row_index = (ctx.selected_row_index - 1) % num_rows;
                break;

            case KEY_DOWN:
                //ctx.selected_row_index = (ctx.selected_row_index + 1) % num_rows;
                break;

            case ENTER_KEY:
                mvprintw(1, 0, "width: %d", (int)width);
                clrtoeol();
                break;

            case 'q':
                endwin();
                exit(0);
        }

        usleep((useconds_t)10000);
    }

    sqlite3_finalize(stmt);
    lpsessiondb_close(sessiondb);

    return 0;
}
