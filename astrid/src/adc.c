#include <signal.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_PULSEAUDIO
#define MA_NO_ALSA
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"

#include "astrid.h"


/* When false, the inner loop exits and begins cleanup */
static volatile int adc_is_running = 1;

/* When false, no frames are written into the shared buffer */
static volatile int adc_is_capturing = 1;

void handle_shutdown(int sig __attribute__((unused))) {
    adc_is_capturing = 0;
    adc_is_running = 0;
}

/* Audio callback: writes a block of frames into 
 * the shared circular buffer and increments the 
 * write position in the buffer. */
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

    /* Open a handle to the system log */
    openlog("astrid-adc", LOG_PID, LOG_USER);

    /* setup signal handlers */
    struct sigaction shutdown_action;
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = 0;

    /* Keyboard interrupt triggers cleanup and shutdown */
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        perror("Could not init SIGINT signal handler");
        exit(1);
    }

    /* Terminate signals also trigger cleanup and shutdown */
    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        perror("Could not init SIGTERM signal handler");
        exit(1);
    }
    
    /* Create a shared memory region to be used as a circular 
     * buffer for renderer processes to read from */
    syslog(LOG_DEBUG, "Creating shared memory buffer for ADC\n");
    if(lpadc_create() < 0) {
        perror("Could not create adcbuf shared memory");
        return 1;
    }

    /* Open the shared memory buffer for writing */
    syslog(LOG_DEBUG, "Opening shared memory buffer for writing\n");
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

    /* init miniaudio device */
    ma_device mad;
    syslog(LOG_DEBUG, "Initializing miniaudio\n");
    if(ma_device_init(NULL, &audioconfig, &mad) != MA_SUCCESS) {
        perror("Runtime Error while attempting to configure miniaudio");
        goto exit_with_error;
    }

    /* start miniaudio device */
    syslog(LOG_DEBUG, "Starting audio callback\n");
    ma_device_start(&mad);

    syslog(LOG_DEBUG, "ADC is now running!\n");
    while(adc_is_running) {
        usleep((useconds_t)1000);
    }

    syslog(LOG_DEBUG, "Exiting normally: shutting down audio\n");
    ma_device_uninit(&mad);

    syslog(LOG_DEBUG, "Exiting normally: cleaning up shared buffer\n");
    lpadc_destroy();
    return 0;

exit_with_error:
    syslog(LOG_DEBUG, "Attempting to clean up after exiting with error\n");
    ma_device_uninit(&mad);
    lpadc_destroy();
    return 1;
}

