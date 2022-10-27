#include "pippi.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    size_t length, pos;
    int channels, samplerate;
    lpbuffer_t * snd;
    char * bytes;
    int src;
    struct stat filestat;

    channels = 2;
    samplerate = 48000;
    src = open("examples/raw.png", O_RDONLY);

    /* fstat check size */
    if(fstat(src, &filestat) == -1) {
        fprintf(stderr, "Could not fstat file.\n");
        exit(1);
    }

    length = filestat.st_size;

    pos = 0;
    bytes = (char *)calloc(length, sizeof(char));
    while(pos < length) {
        pos += read(src, bytes+pos, sizeof(char));
    }

    snd = LPBuffer.create_from_bytes(bytes, length, channels, samplerate);
    LPSoundFile.write("renders/readrawfile-out.wav", snd);
    LPBuffer.destroy(snd);

    close(src);

    return 0;
}
