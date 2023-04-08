#include <signal.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_PULSEAUDIO
#define MA_NO_ALSA
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"

#include "astrid.h"


static volatile int adc_is_running = 1;
static volatile int adc_is_capturing = 1;

void handle_shutdown(int sig __attribute__((unused))) {
    adc_is_capturing = 0;
    adc_is_running = 0;
}

void miniaudio_callback(
    __attribute__((unused)) ma_device * device, 
    __attribute__((unused)) void * pOut, 
       const void * pIn, 
          ma_uint32 count
) {
    lpadcbuf_t * adcbuf = (lpadcbuf_t *)device->pUserData;

    if(adc_is_capturing == 0) return;

    lpadc_write_block(adcbuf, (float *)pIn, (size_t)(count * sizeof(float) * ASTRID_CHANNELS));
}

int main() {
    lpadcbuf_t * adcbuf;

    openlog("astrid-adc", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Setting up signals\n");

    /* setup SIGNINT handler for shutdown */
    struct sigaction action;
    action.sa_handler = handle_shutdown;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if(sigaction(SIGINT, &action, NULL) == -1) {
        perror("Could not init SIGINT signal handler");
        exit(1);
    }

    if(sigaction(SIGTERM, &action, NULL) == -1) {
        perror("Could not init SIGTERM signal handler");
        exit(1);
    }
    
    syslog(LOG_DEBUG, "Creating shared memory\n");
    /* Create shared memory buffer */
    if(lpadc_create() < 0) {
        perror("Could not create adcbuf shared memory");
        return 1;
    }

    adcbuf = lpadc_open();
    if(adcbuf == NULL) {
        perror("Runtime error while attempting to open shared memory buffer in audio callback");
        return 1;
    }

    /* Configure miniaudio for capture mode */
    ma_device_config audioconfig = ma_device_config_init(ma_device_type_capture);
    audioconfig.capture.format = ma_format_f32;
    audioconfig.capture.channels = ASTRID_CHANNELS;
    audioconfig.sampleRate = ASTRID_SAMPLERATE;
    audioconfig.dataCallback = miniaudio_callback;
    audioconfig.pUserData = (void *)adcbuf;

    syslog(LOG_DEBUG, "Starting miniaudio callback\n");
    /* init miniaudio device */
    ma_device mad;
    if(ma_device_init(NULL, &audioconfig, &mad) != MA_SUCCESS) {
        perror("Runtime Error while attempting to configure miniaudio");
        goto exit_with_error;
    }

    /* start miniaudio device */
    ma_device_start(&mad);

    syslog(LOG_DEBUG, "Running\n");
    while(adc_is_running) {
        usleep((useconds_t)1000);
    }

    ma_device_uninit(&mad);
    lpadc_destroy();
    return 0;

exit_with_error:
    ma_device_uninit(&mad);
    lpadc_destroy();
    return 1;
}

