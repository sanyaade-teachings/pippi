#include "astrid.h"

void print_usage(char * program_name) {
    printf("Usage:\n%s <c|r|w|d>\n", program_name);
}

int main(__attribute__((unused)) int argc, char * argv[]) {
    lpbuffer_t * buf;
    char cmd;
    int i;
    float * samples[2];
    char * name = "fake-adc";
    char * out_path = "sample.wav";

    cmd = argv[1][0];
    buf = NULL;

    switch(cmd) {
        case 'c':
            if(lpsampler_create(name, 1000, 2, 48000) < 0) {
                fprintf(stderr, "Could not create buffer\n");
                return 1;
            }
            printf("Created shared memory buffer with name %s\n", name);
            break;

        case 'w':
            samples[0] = calloc(100, sizeof(float));
            samples[1] = calloc(100, sizeof(float));
            memset(samples[0], 0, sizeof(float) * 100);
            memset(samples[1], 0, sizeof(float) * 100);

            for(i=0; i < 100; i++) {
                samples[0][i] = LPRand.rand(0, 0.5f);
                samples[1][i] = -1.f;
            }

            /* write the block into the adc ringbuffer */
            if(lpsampler_write_ringbuffer_block(name, samples, 2, 100) < 0) {
                syslog(LOG_ERR, "Error writing into adc ringbuf\n");
                return 0;
            }
            break;

        case 'r':
            if(lpsampler_aquire(name, &buf) < 0) {
                fprintf(stderr, "Could not aquire buffer\n");
                return 1;
            }

            assert(buf != NULL);
            assert(buf->length > 0);
            LPSoundFile.write(out_path, buf);

            if(lpsampler_release(name) < 0) {
                fprintf(stderr, "Could not release buffer\n");
                return 1;
            }

            printf("Wrote shared memory buffer at name %s to %s\n", name, out_path);
            break;

        case 'd':
            if(lpsampler_destroy(name) < 0) {
                fprintf(stderr, "Could not destroy buffer\n");
                return 1;
            }

            printf("Destroyed shared memory buffer at name %s\n", name);
            break;

        default:
            print_usage(argv[0]);
    }

    return 0;
}
