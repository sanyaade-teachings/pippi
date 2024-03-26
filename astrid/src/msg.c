#include "astrid.h"

int main(int argc, char * argv[]) {
    lpmsg_t msg = {0};

    if(parse_message_from_args(argc, 0, argv, &msg) < 0) {
        fprintf(stderr, "astrid-msg: Could not parse message from args\n");
        return 1;
    }

    syslog(LOG_DEBUG, "Sending message: %s\n", msg.instrument_name);
    syslog(LOG_DEBUG, "             %d (msg.voice_id)\n", (int)msg.voice_id);
    syslog(LOG_DEBUG, "             %d (msg.type)\n", (int)msg.type);

    if(msg.type == LPMSG_SERIAL) {
        if(send_serial_message(msg, msg.instrument_name) < 0) {
            fprintf(stderr, "astrid-msg: Could not send serial message...\n");
            return 1;
        }
    } else {
        if(send_play_message(msg) < 0) {
            fprintf(stderr, "astrid-msg: Could not send play message...\n");
            return 1;
        }
    }

    return 0;
}


