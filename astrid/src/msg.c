#include "astrid.h"

int main(int argc, char * argv[]) {
    lpmsg_t msg = {0};
    if(parse_message_from_args(argc, 0, argv, &msg) < 0) {
        fprintf(stderr, "astrid-msg: Could not parse message from args\n");
        return 1;
    }
    return 0;
}
