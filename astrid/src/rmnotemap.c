#include "astrid.h"

int main(int argc, char * argv[]) {
    int device_id, note, map_index;

    if(argc != 4) {
        fprintf(stderr, "Usage: %s <device_id:int> <note:int> <map_index:int> (argc: %d)\n", argv[0], argc);
        return 1;
    }

    device_id = atoi(argv[1]);
    note = atoi(argv[2]);
    map_index = atoi(argv[3]);

    if(lpmidi_remove_msg_from_notemap(device_id, note, map_index) < 0) {
        fprintf(stderr, "Could not remove msg from notemap\n");
        return 1;
    }

    if(lpmidi_print_notemap(device_id, note) < 0) {
        fprintf(stderr, "Could not print notemap\n");
        return 1;
    }

    return 0;
}
