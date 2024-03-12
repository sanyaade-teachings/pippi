#include "astrid.h"

int main() {
    char * prefix = "buf";
    size_t decoded, val = 123456;
    char encoded[LPKEY_MAXLENGTH];

    lpencode_with_prefix(prefix, val, encoded);
    decoded = lpdecode_with_prefix(encoded);

    printf("val %ld\n", val);
    printf("encoded %s\n", encoded);
    printf("decoded %ld\n", decoded);

    return 0;
}
