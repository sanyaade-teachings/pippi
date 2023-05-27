#include "astrid.h"

int main(int argc, char * argv[]) {
    int bytesread, a, i, length, voice_id;
    size_t instrument_name_length;
    lpmsg_t msg = {0};
    char msgtype;
    char message_params[LPMAXMSG] = {0};
    char instrument_name[LPMAXNAME] = {0};
    lpcounter_t c;

    msgtype = argv[1][0];

    /* Prepare the message param string */
    bytesread = 0;
    for(a=2; a < argc; a++) {
        length = strlen(argv[a]);
        if(a==2) {
            instrument_name_length = (size_t)length;
            memcpy(instrument_name, argv[a], instrument_name_length);
            continue;
        }

        for(i=0; i < length; i++) {
            message_params[bytesread] = argv[a][i];
            bytesread++;
        }
        message_params[bytesread] = SPACE;
        bytesread++;
    }

    /* Set up the message struct */
    strncpy(msg.instrument_name, instrument_name, instrument_name_length);
    strncpy(msg.msg, message_params, bytesread);

    /* Set the message type from the first arg */
    switch(msgtype) {
        case PLAY_MESSAGE:
            msg.type = LPMSG_PLAY;
            break;

        case LOAD_MESSAGE:
            msg.type = LPMSG_LOAD;
            break;

        case STOP_MESSAGE:
            msg.type = LPMSG_STOP;
            break;

        case SHUTDOWN_MESSAGE:
            msg.type = LPMSG_SHUTDOWN;
            break;

        default:
            msg.type = LPMSG_SHUTDOWN;
            break;
    }

    /* Get the voice ID from the voice ID counter */
    if((c.semid = lpipc_getid(LPVOICE_ID_SEMID)) < 0) {
        fprintf(stderr, "Problem trying to get voice ID sempahore handle when constructing play message from command input\n");
        return 1;

    }

    if((c.shmid = lpipc_getid(LPVOICE_ID_SHMID)) < 0) {
        fprintf(stderr, "Problem trying to get voice ID shared memory handle when constructing play message from command input\n");
        return 1;
    }

    if((voice_id = lpcounter_read_and_increment(&c)) < 0) {
        fprintf(stderr, "Problem trying to get voice ID when constructing play message from command input\n");
        return 1;
    }

    msg.voice_id = voice_id;

    /* Send the play message over the message queue */
    if(send_play_message(msg) < 0) {
        fprintf(stderr, "Could not send play message...\n");
        return 1;
    }

    /* Record the voice in the sessiondb */
    if(lpsessiondb_insert_voice(msg) < 0) {
        fprintf(stderr, "Could not insert voice record in sessiondb...\n");
        return 1;
    }

    return 0;
}


