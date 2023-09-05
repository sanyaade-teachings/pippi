#ifndef LPJACKD_H
#define LPJACKD_H

#include "astrid.h"
#include <jack/jack.h>


/* JACK HELPERS
 * ***************/
int print_jack_status(jack_status_t jack_status) {
    syslog(LOG_DEBUG, "JACK STATUS %d\n", (int)jack_status);
    return 0;
}

#endif
