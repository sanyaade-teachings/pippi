#include "astrid.h"

int main(int argc, char * argv[]) {
    int device_id, note;

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <device_id:int> <note:int> (argc: %d)\n", argv[0], argc);
        return 1;
    }

    device_id = atoi(argv[1]);
    note = atoi(argv[2]);

    if(lpmidi_print_notemap(device_id, note) < 0) {
        fprintf(stderr, "Could not print notemap\n");
        return 1;
    }

    return 0;
}
