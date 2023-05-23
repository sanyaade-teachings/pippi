#include "astrid.h"

int main(int argc, char * argv[]) {
    int current_device_id, requested_device_id;

    openlog("astrid-setdeviceid", LOG_PID, LOG_USER);

    if(argc != 2) printf("Usage: %s <device id>\n", argv[0]);

    if((requested_device_id = atoi(argv[1])) == -1) {
        printf("Invalid device id: %s\n", argv[1]);
    }

    current_device_id = lpipc_getid(ASTRID_DEVICEID_PATH);
    printf("Previous device ID: %d\n", current_device_id);
    lpipc_setid(ASTRID_DEVICEID_PATH, requested_device_id);
    printf("Current device ID: %d\n", requested_device_id);

    return 0;
}
