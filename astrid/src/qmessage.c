#include "astrid.h"

int main(int argc, char * argv[]) {
    lpmsg_t msg = {0};

    if(parse_message_from_args(argc, 0, argv, &msg) < 0) {
        fprintf(stderr, "qmessage: Could not parse message from args\n");
        return 1;
    }

    syslog(LOG_DEBUG, "Sending message: %s\n", msg.instrument_name);
    syslog(LOG_DEBUG, "             %d (msg.voice_id)\n", (int)msg.voice_id);
    syslog(LOG_DEBUG, "             %d (msg.type)\n", (int)msg.type);

    if(msg.type == LPMSG_PLAY || msg.type == LPMSG_LOAD || msg.type == LPMSG_TRIGGER) {
        /* Send the play message over the message queue */
        if(send_play_message(msg) < 0) {
            fprintf(stderr, "qmessage: Could not send play message...\n");
            return 1;
        }

        /* Record the voice in the sessiondb */
        if(lpsessiondb_insert_voice(msg) < 0) {
            fprintf(stderr, "qmessage: Could not insert voice record in sessiondb...\n");
            return 1;
        }
    } else {
        /* Send the message to the dac message q */
        if(send_message(msg) < 0) {
            fprintf(stderr, "qmessage: Could not send message...\n");
            return 1;
        }
    }

    return 0;
}


