#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"

#include "astrid.h"


int main(int argc, char * argv[]) {
    int device_id;
    ma_context context;
    ma_uint32 i, playback_device_count, capture_device_count;
    ma_device_info * playback_devices;
    ma_device_info * capture_devices;


    openlog("astrid-getdeviceids", LOG_PID, LOG_USER);
    if(ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        syslog(LOG_ERR, "Error while attempting to initialize miniaudio device context\n");
        return -1;
    }

    if(ma_context_get_devices(&context, &playback_devices, &playback_device_count, &capture_devices, &capture_device_count) != MA_SUCCESS) {
        syslog(LOG_ERR, "Error while attempting to get devices\n");
        return -1;
    }

    if((device_id = lpipc_getid(ASTRID_DEVICEID_PATH)) == -1) {
        /* If there's no device selected, set to the default device */
        lpipc_setid(ASTRID_DEVICEID_PATH, 0);
        device_id = 0;
    }

    for(i = 0; i < playback_device_count; i += 1) {
        if((int)i == device_id) {
            printf("\e[1m%d - %s\e[22m\n", i, playback_devices[i].name);
        } else {
            printf("%d - %s\n", i, playback_devices[i].name);
        }
    }

    ma_context_uninit(&context);

    return 0;
}
