#include <signal.h>
#include <sys/syscall.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DECODING
#include "miniaudio/miniaudio.h"
#include <hiredis/hiredis.h>
#include "pqueue.h"

#include "astrid.h"


static volatile int astrid_is_running = 1;
lpscheduler_t * astrid_scheduler;
sqlite3 * sessiondb;
pqueue_t * msgpq;

/* Callback for SIGINT */
void handle_shutdown(int sig __attribute__((unused))) {
    lpmsg_t msg = {0};
    astrid_is_running = 0;

    syslog(LOG_INFO, "Sending shutdown to message q...\n");
    msg.type = LPMSG_SHUTDOWN;
    if(send_message(msg) < 0) {
        syslog(LOG_ERR, "dac handle_shutdown write: Could not write shutdown message to q. Error: %s\n", strerror(errno));
        exit(1);
    }
}

void finalplay_callback(lpmsg_t msg) {
    syslog(LOG_DEBUG, "FINALPLAY CALLBACK %d (msg.voice_id)\n", (int)msg.voice_id);
    lpsessiondb_mark_voice_stopped(sessiondb, msg.voice_id, msg.count);
    syslog(LOG_DEBUG, "FINALPLAY CALLBACK marked stopped\n");
}

void noop_callback(__attribute__((unused)) lpmsg_t msg) {}

static int msgpq_cmp_pri(double next, double curr) {
    return (next < curr);
}

static double msgpq_get_pri(void * a) {
    return ((lpmsgpq_node_t *)a)->timestamp;
}

static void msgpq_set_pri(void * a, double timestamp) {
    ((lpmsgpq_node_t *)a)->timestamp = timestamp;
}

static size_t msgpq_get_pos(void * a) {
    return ((lpmsgpq_node_t *)a)->pos;
}

static void msgpq_set_pos(void * a, size_t pos) {
    ((lpmsgpq_node_t *)a)->pos = pos;
}


void * message_scheduler_pq(__attribute__((unused)) void * arg) {
    lpmsg_t * msg;
    lpmsgpq_node_t * node;
    void * d;
    double now;

    now = 0;
    syslog(LOG_DEBUG, " MPQ           STARTING\n");

    while(astrid_is_running) {
        /* peek into the queue */
        d = pqueue_peek(msgpq);

        /* No messages have arrived */
        if(d == NULL) {
            usleep((useconds_t)500);
            continue;
        }

        /* There is a message! */
        node = (lpmsgpq_node_t *)d;
        msg = node->msg;

        if(msg->type == LPMSG_SHUTDOWN) {
            free(msg);
            free(node);
            break;
        }

        /* Get now */
        if(lpscheduler_get_now_seconds(&now) < 0) {
            syslog(LOG_CRIT, "Error getting now in message scheduler\n");
            exit(1);
        }

        /* If msg timestamp is in the future, 
         * sleep for a bit and then try again */
        if(msg->timestamp > now) {
            usleep((useconds_t)500);
            continue;
        }

        syslog(LOG_DEBUG, " MESSAGE PQ    %s MSG PRIORITY QUEUE SENDING MESSAGE %s\n", msg->instrument_name, msg->msg);
        syslog(LOG_DEBUG, " MESSAGE PQ    %s (msg.instrument_name)\n", msg->instrument_name);
        syslog(LOG_DEBUG, " MESSAGE PQ    %d (msg.voice_id)\n", (int)msg->voice_id);
        syslog(LOG_DEBUG, " MESSAGE PQ    %f (msg.timestamp)\n", msg->timestamp);
        syslog(LOG_DEBUG, " MESSAGE PQ    %d (msg.onset_delay)\n", (int)msg->onset_delay);
        syslog(LOG_DEBUG, " MESSAGE PQ    %f (now)\n", now);

        /* Send it along to the instrument message fifo */
        if(send_play_message(*msg) < 0) {
            syslog(LOG_ERR, "Error sending play message from message priority queue\n");
            usleep((useconds_t)500);
            continue;
        }

        /* And remove it from the pq */
        if(pqueue_remove(msgpq, d) < 0) {
            syslog(LOG_ERR, "pqueue_remove: problem removing message from the pq\n");
            usleep((useconds_t)500);
        }

        /* TODO do this somewhere else maybe? */
        free(msg);
        free(node);

        syslog(LOG_DEBUG, " MESSAGE PQ     DONE W/SENDING MESSAGE & CLEANUP\n");
    }

    syslog(LOG_INFO, "Message scheduler pq thread shutting down...\n");
    /* Clean up the pq: TODO, check for orphan messages? */
    pqueue_free(msgpq);
    syslog(LOG_DEBUG, "Message scheduler pq thread exiting...\n");
    return 0;
}

void * message_feed(__attribute__((unused)) void * arg) {
    int qfd;
    lpmsg_t msg = {0};
    lpmsg_t * msgout;
    lpmsgpq_node_t * d;

    if((qfd = astrid_msgq_open()) < 0) {
        syslog(LOG_CRIT, "Could not open msgq: %s\n", strerror(errno));
        exit(1);        
    }

    syslog(LOG_INFO, "Message feed: Waiting for messages...\n");

    /* Wait for messages on the msgq fifo */
    while(astrid_is_running) {
        if(astrid_msgq_read(qfd, &msg) < 0) {
            syslog(LOG_ERR, "Error while fetching message during msgq loop: %s\n", strerror(errno));
            continue;
        }

        syslog(LOG_DEBUG, " MFT           %s MSG FEED THREAD GOT MESSAGE %s\n", msg.instrument_name, msg.msg);
        syslog(LOG_DEBUG, " MFT           %s (msg.instrument_name)\n", msg.instrument_name);
        syslog(LOG_DEBUG, " MFT           %d (msg.voice_id)\n", (int)msg.voice_id);
        syslog(LOG_DEBUG, " MFT           %d (msg.type)\n", msg.type);
        syslog(LOG_DEBUG, " MFT           %f (msg.timestamp)\n", msg.timestamp);
        syslog(LOG_DEBUG, " MFT           %d (msg.onset_delay)\n", (int)msg.onset_delay);

        d = (lpmsgpq_node_t *)calloc(1, sizeof(lpmsgpq_node_t));
        msgout = (lpmsg_t *)calloc(1, sizeof(lpmsg_t));
        memcpy(msgout, &msg, sizeof(lpmsg_t));
        d->msg = msgout;
        d->timestamp = msg.timestamp;

        syslog(LOG_DEBUG, " MFT Inserting message into pq for scheduling\n");

        if(pqueue_insert(msgpq, (void *)d) < 0) {
            syslog(LOG_ERR, "Error while inserting message into pq during msgq loop: %s\n", strerror(errno));
            continue;
        }

        /* Exit the loop on shutdown message after sending 
         * it along to the priority queue as well */
        if(msg.type == LPMSG_SHUTDOWN) break;
    }

    syslog(LOG_INFO, "Message feed shutting down...\n");
    if(qfd != -1) astrid_msgq_close(qfd);

    return 0;
}


/* This callback runs in a thread started 
 * just before the audio callback is started.
 *
 * It waits on the buffer queue for new buffers, 
 * and then sends them to the scheduler.
 */
void * buffer_feed(__attribute__((unused)) void * arg) {
    redisContext * redis_ctx;
    redisReply * redis_reply;
    lpbuffer_t * buf;
    lpmsg_t msg = {0};
    double now, delay;
    size_t delay_frames;

    struct timeval redis_timeout = {15, 0};
    size_t callback_delay = 0;

    syslog(LOG_INFO, "Buffer feed starting up...\n");
    redis_ctx = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    if(redis_ctx == NULL) {
        syslog(LOG_CRIT, "Could not start connection to redis.\n");
        exit(1);
    }

    if(redis_ctx->err) {
        syslog(LOG_CRIT, "There was a problem while connecting to redis. %s\n", redis_ctx->errstr);
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
    syslog(LOG_INFO, "Waiting for buffers...\n");
    while(astrid_is_running && redisGetReply(redis_ctx, (void *)&redis_reply) == REDIS_OK) {
        if(redis_reply->type == REDIS_REPLY_ARRAY) {
            syslog(LOG_DEBUG, "Got message on redis buffer channel...\n");
            if(redis_reply->element[2]->str[0] == 's') {
                syslog(LOG_INFO, "Buffer feed got shutdown message\n");
                syslog(LOG_INFO, "    message: %s\n", redis_reply->element[2]->str);
                break;
            }
            buf = deserialize_buffer(redis_reply->element[2]->str, &msg);

            /* Increment the message count */
            msg.count += 1;

            /* Schedule the buffer for playback */
            scheduler_schedule_event(astrid_scheduler, buf, buf->onset);

            /* Mark the voice active on the first render and 
             * increment the render count if looping */
            if(msg.count == 1) {
                lpsessiondb_mark_voice_active(sessiondb, msg.voice_id);
            } else if(msg.count > 1 && buf->is_looping) {
                lpsessiondb_increment_voice_render_count(sessiondb, msg.voice_id, msg.count);
            }

            /* If the buffer is flagged to loop, schedule the next render 
             * by placing the message onto the message scheduling priority 
             * queue with a timestamp for sending the render message 70% 
             * into the buffer playback, with an onset delay for the buffer 
             * making up the last 30%. TODO: measure jitter when scheduling 
             * in regular intervals. */
            if(buf->is_looping == 1) {
                /* Get now to schedule the next render */
                if(lpscheduler_get_now_seconds(&now) < 0) {
                    syslog(LOG_ERR, "Could not get now seconds for loop retriggering\n");
                    continue;
                }

                delay_frames = (size_t)(buf->length * 0.7f);
                delay = (delay_frames / (double)buf->samplerate);
                msg.timestamp = now + delay;
                msg.onset_delay = buf->length - delay_frames;

                if(send_message(msg) < 0) {
                    syslog(LOG_ERR, "Could not schedule message for loop retriggering\n");
                    continue;
                }
            }

        }
        freeReplyObject(redis_reply);
    }

    syslog(LOG_INFO, "Buffer feed shutting down...\n");

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
        lpscheduler_tick(ctx->s);
        for(c=0; c < ASTRID_CHANNELS; c++) {
            *out++ = (float)ctx->s->current_frame[c];
        }
    }
}

int cleanup(
    ma_device * playback, 
    lpdacctx_t * ctx, 
    pthread_t buffer_feed_thread, 
    pthread_t message_feed_thread, 
    pthread_t message_scheduler_pq_thread, 
    sqlite3 * sessiondb
) {
    redisContext * redis_ctx;
    redisReply * redis_reply;
    struct timeval redis_timeout = {15, 0};

    redis_ctx = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    if(redis_ctx == NULL) {
        syslog(LOG_ERR, "Could not start connection to redis.\n");
        exit(1);
    }

    if(redis_ctx->err) {
        syslog(LOG_ERR, "There was a problem while connecting to redis. %s\n", redis_ctx->errstr);
        exit(1);
    }

    syslog(LOG_INFO, "Sending shutdown to buffer thread...\n");
    astrid_is_running = 0;
    redis_reply = redisCommand(redis_ctx, "PUBLISH astridbuffers shutdown");
    if(redis_reply->str != NULL) syslog(LOG_INFO, "astridbuffers shutdown result: %s\n", redis_reply->str); 
    freeReplyObject(redis_reply);
    if(redis_ctx != NULL) redisFree(redis_ctx); 

    syslog(LOG_DEBUG, "Joining with buffer thread...\n");
    if(pthread_join(buffer_feed_thread, NULL) != 0) {
        syslog(LOG_ERR, "Error while attempting to join with buffer thread\n");
    }

    syslog(LOG_INFO, "Closing audio thread...\n");
    if(playback != NULL) ma_device_uninit(playback);

    syslog(LOG_INFO, "Cleaning up scheduler...\n");
    if(ctx != NULL) scheduler_destroy(ctx->s);

    syslog(LOG_INFO, "Closing sessiondb...\n");
    if(sessiondb != NULL) lpsessiondb_close(sessiondb);

    syslog(LOG_INFO, "Freeing astrid context...\n");
    if(ctx != NULL) free(ctx);

    syslog(LOG_DEBUG, "Joining with message thread...\n");
    if(pthread_join(message_feed_thread, NULL) != 0) {
        syslog(LOG_ERR, "Error while attempting to join with message thread\n");
    }

    syslog(LOG_DEBUG, "Joining with message scheduler pq thread...\n");
    if(pthread_join(message_scheduler_pq_thread, NULL) != 0) {
        syslog(LOG_ERR, "Error while attempting to join with message scheduler pq thread\n");
    }

    syslog(LOG_INFO, "Done with cleanup!\n");

    closelog();

    return 0;
}

int main() {
    struct sigaction shutdown_action;
    lpdacctx_t * ctx;
    lpcounter_t voice_id_counter;
    pthread_t buffer_feed_thread;
    pthread_t message_feed_thread;
    pthread_t message_scheduler_pq_thread;
    int device_id;
    ma_uint32 playback_device_count, capture_device_count;
    ma_device playback;
    ma_device_info * playback_devices;
    ma_device_info * capture_devices;

    sessiondb = NULL;
    ctx = NULL;
    openlog("astrid-dac", LOG_PID, LOG_USER);

    /* Set shutdown signal handlers */
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = 0;
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGINT signal handler. Error: %s\n", strerror(errno));
        exit(1);
    }

    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGTERM signal handler. Error: %s\n", strerror(errno));
        exit(1);
    }

    /* init scheduler and ctx 
     * 
     * The scheduler is shared between the miniaudio callback 
     * and astrid buffer feed threads. The buffer feed thread 
     * may schedule buffers by adding them to the internal linked 
     * list in the scheduler. The miniaudio callback may read from 
     * the linked list, increment counts in playing buffers and 
     * flag buffers as having playback completed. 
     **/
    astrid_scheduler = scheduler_create(1, ASTRID_CHANNELS, ASTRID_SAMPLERATE);
    ctx = (lpdacctx_t*)LPMemoryPool.alloc(1, sizeof(lpdacctx_t));
    ctx->s = astrid_scheduler;
    ctx->channels = ASTRID_CHANNELS;
    ctx->samplerate = ASTRID_SAMPLERATE;

    /* Set up shared memory IPC for voice IDs */
    if(lpcounter_create(&voice_id_counter) < 0) {
        syslog(LOG_ERR, "Could not initialize voice ID shared memory. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    syslog(LOG_DEBUG, "voice counter init, shmid: %d\n", (int)voice_id_counter.shmid);
    syslog(LOG_DEBUG, "voice counter init, semid: %d\n", (int)voice_id_counter.semid);

    /* Store a reference to the shared memory in well known files */
    if(lpipc_setid(LPVOICE_ID_SHMID, voice_id_counter.shmid) < 0) {
        syslog(LOG_ERR, "Could not store voice ID shmid. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    if(lpipc_setid(LPVOICE_ID_SEMID, voice_id_counter.semid) < 0) {
        syslog(LOG_ERR, "Could not store voice ID shmid. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Initialize the sessiondb */
    if(lpsessiondb_create(&sessiondb) < 0) {
        syslog(LOG_ERR, "Could not initialize the sessiondb. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Create the message priority queue */
    if((msgpq = pqueue_init(LPMSG_MAX_PQ, msgpq_cmp_pri, msgpq_get_pri, msgpq_set_pri, msgpq_get_pos, msgpq_set_pos)) == NULL) {
        syslog(LOG_ERR, "Could not initialize message priority queue. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Start message feed thread */
    if(pthread_create(&message_feed_thread, NULL, message_feed, ctx) != 0) {
        syslog(LOG_ERR, "Could not initialize message feed thread. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Start message pq thread */
    if(pthread_create(&message_scheduler_pq_thread, NULL, message_scheduler_pq, NULL) != 0) {
        syslog(LOG_ERR, "Could not initialize message scheduler pq thread. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Start buffer feed thread */
    if(pthread_create(&buffer_feed_thread, NULL, buffer_feed, ctx) != 0) {
        syslog(LOG_ERR, "Could not initialize buffer feed thread. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Set up the miniaudio device context */
    ma_context audio_device_context;
    if (ma_context_init(NULL, 0, NULL, &audio_device_context) != MA_SUCCESS) {
        syslog(LOG_ERR, "Error while attempting to initialize miniaudio device context\n");
        goto exit_with_error;
    }

    /* Populate it with some devices */
    if(ma_context_get_devices(&audio_device_context, &playback_devices, &playback_device_count, &capture_devices, &capture_device_count) != MA_SUCCESS) {
        syslog(LOG_ERR, "Error while attempting to get devices\n");
        return -1;
    }

    /* Get the selected device ID */
    device_id = lpipc_getid(ASTRID_DEVICEID_PATH);
    if(device_id < 0) {
        /* If no device has been selected, set it to the default device */
        device_id = 0;
        lpipc_setid(ASTRID_DEVICEID_PATH, device_id);
    }

    /* Setup and start miniaudio in playback mode */
    ma_device_config audioconfig = ma_device_config_init(ma_device_type_playback);
    audioconfig.playback.format = ma_format_f32;
    audioconfig.playback.channels = ASTRID_CHANNELS;
    audioconfig.playback.pDeviceID = &playback_devices[device_id].id;
    audioconfig.sampleRate = ASTRID_SAMPLERATE;
    audioconfig.dataCallback = miniaudio_callback;
    audioconfig.pUserData = ctx;

    syslog(LOG_INFO, "Opening device ID %d\n", device_id);
    if(ma_device_init(&audio_device_context, &audioconfig, &playback) != MA_SUCCESS) {
        syslog(LOG_ERR, "Error while attempting to configure device %d for playback\n", device_id);
        goto exit_with_error;
    }

    syslog(LOG_INFO, "Audio callback starting...\n");
    ma_device_start(&playback);

    syslog(LOG_INFO, "Astrid DAC is starting...\n");
    while(astrid_is_running) {
        /* Twiddle thumbs */
        usleep((useconds_t)1000);
        /*LPScheduler.empty(ctx->s);*/
    }

    return cleanup(&playback, ctx, buffer_feed_thread, message_feed_thread, message_scheduler_pq_thread, sessiondb);

exit_with_error:
    cleanup(&playback, ctx, buffer_feed_thread, message_feed_thread, message_scheduler_pq_thread, sessiondb);
    syslog(LOG_ERR, "Exited with error\n");
    return 1;
}


