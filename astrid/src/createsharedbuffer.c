#include "astrid.h"

void print_usage(char * program_name) {
    printf("Usage:\n%s <c|d> /path/to/buffer/handle (path/to/out.wav)\n", program_name);
}

int main(int argc, char * argv[]) {
    lpipc_buffer_t * buf;
    lpbuffer_t * out;
    char * id_path;
    char * out_path;
    char cmd;

    printf("argc %d\n", argc);

    if(argc < 3 || argc > 4) {
        print_usage(argv[0]);
        return 1;
    }

    cmd = argv[1][0];
    id_path = argv[2];

    buf = NULL;
    out = NULL;

    switch(cmd) {
        case 'c':
            if(lpipc_buffer_create(id_path, 48000 * 10, 2, 48000) < 0) {
                fprintf(stderr, "Could not create buffer\n");
                return 1;
            }

            printf("Created shared memory buffer with handle %s\n", id_path);
            break;

        case 'r':
            if(argc < 4) {
                print_usage(argv[0]);
                return 1;
            }

            out_path = argv[3];

            if(lpipc_buffer_aquire(id_path, &buf) < 0) {
                fprintf(stderr, "Could not aquire buffer\n");
                return 1;
            }

            assert(buf != NULL);
            assert(buf->length > 0);

            lpipc_buffer_tolpbuffer(buf, &out);

            assert(out != NULL);
            assert(out->length > 0);

            LPSoundFile.write(out_path, out);

            if(lpipc_buffer_release(id_path) < 0) {
                fprintf(stderr, "Could not release buffer\n");
                return 1;
            }

            printf("Wrote shared memory buffer at handle %s to %s\n", id_path, out_path);
            break;

        case 'x':
            if(lpipc_buffer_release(id_path) < 0) {
                fprintf(stderr, "Could not release buffer\n");
                return 1;
            }

            printf("Released shared memory buffer at handle %s\n", id_path);
            break;


        case 'd':
            if(lpipc_buffer_destroy(id_path) < 0) {
                fprintf(stderr, "Could not destroy buffer\n");
                return 1;
            }

            printf("Destroyed shared memory buffer at handle %s\n", id_path);
            break;

        default:
            print_usage(argv[0]);
    }

    return 0;
}
