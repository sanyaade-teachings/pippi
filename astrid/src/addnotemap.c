#include "astrid.h"

int main(int argc, char * argv[]) {
    int device_id, note;
    lpmsg_t msg = {0};

    if(argc < 3) {
        fprintf(stderr, "Usage: %s <device_id:int> <note:int> <msgtype:str> <instrument:str> [<params:str>] (argc: %d)\n", argv[0], argc);
        return 1;
    }

    device_id = atoi(argv[1]);
    note = atoi(argv[2]);

    if(parse_message_from_args(argc, 2, argv, &msg) < 0) {
        fprintf(stderr, "addnotemap: Could not parse message from args\n");
        return 1;
    }

    if(lpmidi_add_msg_to_notemap(device_id, note, msg) < 0) {
        fprintf(stderr, "addnotemap: Could not add msg to notemap\n");
        return 1;
    }

    if(lpmidi_print_notemap(device_id, note) < 0) {
        fprintf(stderr, "addnotemap: Could not print notemap\n");
        return 1;
    }

    return 0;
}
