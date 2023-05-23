#include "astrid.h"
#include <sqlite3.h>

static int callback(void * unused, int argc, char ** argv, char ** colname) {
    int i;
    for(i=0; i < argc; i++) {
        printf("%s=%s\n", colname[i], argv[i] ? argv[i] : "None");
    }
    return 0;
}

int main() {
    sqlite3 * db;
    char * err = 0;

    char * sql = "create table voices (started, ended, id, instrument_name, params, message_sent, render_count);";

    unlink(ASTRID_SESSIONDB_PATH);

    if(sqlite3_open(ASTRID_SESSIONDB_PATH, &db) > 0) {
        printf("Could not open db at path: %s\n", ASTRID_SESSIONDB_PATH);
        return 1;
    }

    if(sqlite3_exec(db, sql, callback, 0, &err) != SQLITE_OK) {
        printf("Could not exec sql statement: %s\n", sql);
        return 1;
    }

    sqlite3_close(db);

    return 0;
}
