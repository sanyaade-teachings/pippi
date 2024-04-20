#include "astrid.h"

int main() {
    ssize_t voice_id;

    if((voice_id = (ssize_t)lpcounter_read_and_increment("voiceid")) < 0) {
        perror("lpcounter_read_and_increment");
        return 1;
    }

    printf("Voice ID: %d\n", (int)voice_id);

    return 0;
}
