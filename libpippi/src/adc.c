#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_PULSEAUDIO
#define MA_NO_ALSA
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"
#include <hiredis/hiredis.h>

#include "pippi.h"
#include "astrid.h"
#include "adc.h"

#define ADC_LENGTH 4800000

static volatile int adc_is_running = 1;
static volatile int adc_is_capturing = 1;

void handle_shutdown(int) {
    adc_is_running = 0;
}

void miniaudio_callback(
        ma_device * device, 
    __attribute__((unused)) void * pOut, 
       const void * pIn, 
          ma_uint32 count
) {
    ma_uint32 i;
    float * in;
    int c;
    lpadcbuf_t * adcbuf;
    lpfloat_t sample;

    adcbuf = (lpadcbuf_t *)device->pUserData;
    in = (float *)pIn;

    if(adc_is_capturing == 0) return;

    for(i=0; i < count; i++) {
        for(c=0; c < ASTRID_CHANNELS; c++) {
            sample = (lpfloat_t)*in++;
            lpadc_write_sample(adcbuf, sample, c, 0);
        }

        lpadc_increment_pos(adcbuf);
    }
}

int main() {
    lpadcbuf_t * adcbuf;

    redisContext * status_redis;
    redisReply * status_reply;

    /* setup SIGNINT handler for shutdown */
    struct sigaction action;
    action.sa_handler = handle_shutdown;
    sigemptyset(&action.sa_mask);
    sigemptyset(&action.sa_flags);
    if(sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr, "Could not init signal handler.\n");
        goto exit_with_error;
    }

    /* Connect to redis for status polling */
    struct timeval redis_timeout = {15, 0};
    status_redis = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    
    if(status_redis == NULL) {
        fprintf(stderr, "Could not start connection to redis.\n");
        goto exit_with_error;
    }

    if(status_redis->err) {
        fprintf(stderr, "There was a problem while connecting to redis. %s\n", status_redis->errstr);
        goto exit_with_error;
    }

    /* Open or create shared memory buffer */
    adcbuf = lpadc_open_for_writing();

    /* Configure miniaudio for capture mode */
    ma_device_config audioconfig = ma_device_config_init(ma_device_type_capture);
    audioconfig.capture.format = ma_format_f32;
    audioconfig.capture.channels = ASTRID_CHANNELS;
    audioconfig.sampleRate = ASTRID_SAMPLERATE;
    audioconfig.dataCallback = miniaudio_callback;
    audioconfig.pUserData = adcbuf;

    /* init miniaudio device */
    ma_device mad;
    if(ma_device_init(NULL, &audioconfig, &mad) != MA_SUCCESS) {
        fprintf(stderr, "Runtime Error while attempting to configure miniaudio\n");
        goto exit_with_error;
    }

    /* Set capturing flag to ON */
    status_reply = redisCommand(status_redis, "SET adc_is_capturing 1");
    freeReplyObject(status_reply);

    /* start miniaudio device */
    ma_device_start(&mad);

    while(adc_is_running) {
        usleep((useconds_t)10000);
        status_reply = redisCommand(status_redis, "GET adc_is_capturing");
        adc_is_capturing = atoi(status_reply->str);
        if(adc_is_capturing == 0) printf("adc_is_capturing %s\n", status_reply->str);
        freeReplyObject(status_reply);
        /*printf("adcbuf writepos %f\n", (float)lpadc_get_pos(adcbuf));*/
    }

    redisFree(status_redis);
    ma_device_uninit(&mad);
    lpadc_close(adcbuf);
    lpadc_destroy(adcbuf);
    return 0;

exit_with_error:
    if(status_redis != NULL) redisFree(status_redis);
    ma_device_uninit(&mad);
    lpadc_close(adcbuf);
    lpadc_destroy(adcbuf);
    return 1;
}

