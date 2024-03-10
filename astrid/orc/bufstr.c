#include "astrid.h"

int main() {
    char * prefix = "buf";
    size_t val = 123456;
    char encoded[sizeof(size_t) + strlen(prefix)];

    lpencode_with_prefix(prefix, val, encoded);

    printf("val %ld\n", val);
    printf("encoded %s\n", encoded);

    return 0;
}
