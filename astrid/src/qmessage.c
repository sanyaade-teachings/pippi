#include "astrid.h"

int main(int argc, char * argv[]) {
    int bytesread, a, i, length;
    size_t instrument_name_length;
    lpmsg_t msg = {0};
    char message_params[LPMAXMSG] = {0};
    char instrument_name[LPMAXNAME] = {0};
    lpcounter_t c;

    bytesread = 0;
    for(a=1; a < argc; a++) {
        length = strlen(argv[a]);
        if(a==1) {
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

    strncpy(msg.instrument_name, instrument_name, instrument_name_length);
    strncpy(msg.msg, message_params, bytesread);

    c.semid = lpipc_getid(LPVOICE_ID_SEMID);
    c.shmid = lpipc_getid(LPVOICE_ID_SHMID);
    if((msg.voice_id = lpcounter_read_and_increment(&c)) < 0) {
        fprintf(stderr, "Problem trying to get voice ID when constructing play message from command input\n");
        return 1;
    }

    if(send_play_message(msg) < 0) {
        fprintf(stderr, "Could not send play message...\n");
        return 1;
    }

    return 0;
}


