#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_PULSEAUDIO
#define MA_NO_ALSA
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"
#include <hiredis/hiredis.h>

#include "pippi.h"
#include "astrid.h"
#include "dac.h"

static volatile int astrid_is_running = 1;
int astrid_channels = 2;
lpscheduler_t * astrid_scheduler;

/* Callback for SIGINT */
void handle_shutdown(int) {
    astrid_is_running = 0;
}

/* This callback fires when a buffer in the 
 * mixer has finished playback, and it originated 
 * from an instrument with loop enabled.
 **/
void retrigger_callback(void * arg) {
    char * instrument_name;
    instrument_name = (char *)arg;
    send_play_message(instrument_name);
}

void noop_callback(void * arg) {}

/* This callback runs in a thread started 
 * just before the audio callback is started.
 *
 * It waits on the buffer queue for new buffers, 
 * and then sends them to the scheduler.
 */
void * buffer_feed(void * arg) {
    redisContext * redis_ctx;
    redisReply * redis_reply;
    lpbuffer_t * buf;
    char * buf_instrument_name;
    struct timeval redis_timeout = {15, 0};

    redis_ctx = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    if(redis_ctx == NULL) {
        fprintf(stderr, "Could not start connection to redis.\n");
        exit(1);
    }

    if(redis_ctx->err) {
        fprintf(stderr, "There was a problem while connecting to redis. %s\n", redis_ctx->errstr);
        exit(1);
    }

    /* Subscribe to the buffer queue. 
     * This is the only thread subscribed
     * to the buffer queue as a reader.
     */
    redis_reply = redisCommand(redis_ctx, "SUBSCRIBE astridbuffers");
    if(redis_reply->str != NULL) printf("subscribe result: %s\n", redis_reply->str); 
    freeReplyObject(redis_reply);

    /* Wait on buffers from the queue */
    while(astrid_is_running && redisGetReply(redis_ctx, (void *)&redis_reply) == REDIS_OK) {
        if(redis_reply->type == REDIS_REPLY_ARRAY) {
            buf_instrument_name = NULL;
            buf = deserialize_buffer(redis_reply->element[2]->str, &buf_instrument_name);
            if(buf->is_looping == 1) {
                LPScheduler.schedule_event(astrid_scheduler, buf, buf->onset, retrigger_callback, buf_instrument_name);
            } else {
                LPScheduler.schedule_event(astrid_scheduler, buf, buf->onset, noop_callback, NULL);
            }
        }
        freeReplyObject(redis_reply);
    }

    if(redis_ctx != NULL) redisFree(redis_ctx); 
    return 0;
}

/* This callback runs in a thread managed by miniaudio.
 * It asks for samples from the mixer inside the scheduler 
 * and sends the mixed audio to the soundcard.
 */
void miniaudio_callback(
        ma_device * device, 
             void * pOut, 
    __attribute__((unused)) const void * pIn, 
          ma_uint32 count
) {
    ma_uint32 i;
    float * out;
    int c;
    lpdacctx_t * ctx;

    ctx = (lpdacctx_t *)device->pUserData;
    out = (float *)pOut;

    for(i=0; i < count; i++) {
        LPScheduler.tick(ctx->s);
        for(c=0; c < astrid_channels; c++) {
            *out++ = (float)ctx->s->current_frame[c];
        }
    }
}

int main() {
    char * _astrid_channels;
    struct sigaction shutdown_action;
    lpdacctx_t * ctx;
    int buffer_feed_thread_id;

    /* Get channels from ENV */
    _astrid_channels = getenv("ASTRID_CHANNELS");
    if(_astrid_channels != NULL) {
        astrid_channels = atoi(_astrid_channels);
    }

    /* Setup shutdown handler for ctl-c */
    shutdown_action.sa_handler = handle_shutdown;
    sigaction(SIGINT, &shutdown_action, NULL);

    /* init scheduler and ctx */
    astrid_scheduler = LPScheduler.create(astrid_channels);
    ctx = (lpdacctx_t*)LPMemoryPool.alloc(1, sizeof(lpdacctx_t));
    ctx->s = astrid_scheduler;
    ctx->channels = astrid_channels;
    ctx->samplerate = ASTRID_SAMPLERATE;

    /* Setup and start buffer feed thread */
    buffer_feed_thread_id = -1;
    if(pthread_create(&buffer_feed_thread_id, NULL, buffer_feed, ctx) != 0) {
        fprintf(stderr, "Could not initialize renderer thread\n");
        goto exit_with_error;
    }

    /* Setup and start miniaudio in duplex mode */
    ma_device_config audioconfig = ma_device_config_init(ma_device_type_duplex);
    audioconfig.playback.format = ma_format_f32;
    audioconfig.playback.channels = astrid_channels;
    audioconfig.sampleRate = ASTRID_SAMPLERATE;
    audioconfig.dataCallback = miniaudio_callback;
    audioconfig.pUserData = ctx;

    ma_device playback;
    if(ma_device_init(NULL, &audioconfig, &playback) != MA_SUCCESS) {
        fprintf(stderr, "Error while attempting to configure miniaudio for playback\n");
        goto exit_with_error;
    }

    printf("Playback started\n");
    ma_device_start(&playback);

    printf("Running...\n");
    while(astrid_is_running) {
        /* Twiddle thumbs */
        usleep((useconds_t)10000);
    }

    if(buffer_feed_thread_id > 0) {
        printf("Trying to close buffer thread...\n");
        pthread_join(buffer_feed_thread_id, NULL);
        printf("Buffer thread closed\n");
    }

    printf("Trying to close audio thread...\n");
    ma_device_uninit(&playback);
    printf("Audio thread closed\n");

    printf("Trying to unload scheduler...\n");
    LPScheduler.destroy(ctx->s);
    printf("Scheduler unloaded\n");

    printf("Trying to destroy context...\n");
    free(ctx);
    printf("Context destroyed\n");
    printf("Done!\n");
    return 0;

exit_with_error:
    ma_device_uninit(&playback);
    if(buffer_feed_thread_id > 0) {
        pthread_join(buffer_feed_thread_id, NULL);
    }
    printf("Exited with error\n");
    return 1;
}


