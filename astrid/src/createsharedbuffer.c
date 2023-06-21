#include "astrid.h"

void print_usage(char * program_name) {
    printf("Usage:\n%s <c|r|d> <shmid:int> </path/to/buffer/handle:str> (<path/to/out.wav:str>)\n", program_name);
}

int main(int argc, char * argv[]) {
    int buffer_shmid;
    lpipc_buffer_t * buf;
    lpbuffer_t * out;
    char * id_path;
    char * out_path;
    char cmd;

    printf("argc %d\n", argc);

    if(argc < 4 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }

    cmd = argv[1][0];
    buffer_shmid = atoi(argv[2]);
    id_path = argv[3];

    buf = NULL;
    out = NULL;

    switch(cmd) {
        case 'c':
            if((buffer_shmid = lpipc_buffer_create(id_path, 48000 * 10, 2, 48000)) < 0) {
                fprintf(stderr, "Could not create buffer\n");
                return 1;
            }

            printf("Created shared memory buffer with handle %s\n", id_path);
            printf("buffer_shmid: %d\n", buffer_shmid);
            break;

        case 'r':
            if(argc < 5) {
                print_usage(argv[0]);
                return 1;
            }

            out_path = argv[4];

            if(lpipc_buffer_aquire(id_path, &buf, buffer_shmid) < 0) {
                fprintf(stderr, "Could not aquire buffer\n");
                return 1;
            }

            assert(buf != NULL);
            assert(buf->length > 0);

            lpipc_buffer_tolpbuffer(buf, &out);

            assert(out != NULL);
            assert(out->length > 0);

            LPSoundFile.write(out_path, out);

            if(lpipc_buffer_release(id_path, (void *)buf) < 0) {
                fprintf(stderr, "Could not release buffer\n");
                return 1;
            }

            printf("Wrote shared memory buffer at handle %s to %s\n", id_path, out_path);
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
