#include <hiredis/hiredis.h>
#include <jack/jack.h>
#include "astrid.h"
#include "jack_helpers.h"


static volatile int astrid_is_running = 1;
jack_port_t * astrid_outport1, * astrid_outport2;
jack_client_t * jack_client;
lpscheduler_t * astrid_scheduler;
sqlite3 * sessiondb;

/* Callback for SIGINT */
void handle_shutdown(__attribute__((unused)) int sig) {
    astrid_is_running = 0;
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
    double processing_time_so_far, onset_delay_in_seconds;
    double now = 0;

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
            if(redis_reply->element[2]->str[0] == 's') {
                syslog(LOG_INFO, "Buffer feed got shutdown message\n");
                syslog(LOG_INFO, "    message: %s\n", redis_reply->element[2]->str);
                break;
            }

            if((buf = deserialize_buffer(redis_reply->element[2]->str, &msg)) == NULL) {
                syslog(LOG_ERR, "DAC could not deserialize buffer. Error: (%d) %s\n", errno, strerror(errno));
                continue;
            }

            /* Increment the message count */
            /*msg.count += 1;*/

            /* Get now */
            if(lpscheduler_get_now_seconds(&now) < 0) {
                syslog(LOG_ERR, "Could not get now seconds for loop retriggering\n");
                now = 0;
            }

            /*
            syslog(LOG_DEBUG, "DAC: Message type %d for %s arrived in buffer feed\n", msg.type, msg.instrument_name);
            syslog(LOG_DEBUG, "initiated=%f scheduled=%f voice_id=%ld\n", msg.initiated, msg.scheduled, msg.voice_id);
            */

            processing_time_so_far = now - msg.initiated;
            onset_delay_in_seconds = msg.scheduled - processing_time_so_far;
            if(onset_delay_in_seconds < 0) onset_delay_in_seconds = 0.f;

            msg.onset_delay = (size_t)(onset_delay_in_seconds * ASTRID_SAMPLERATE);

            syslog(LOG_INFO, "\n\nmsg.onset_delay %ld\n", msg.onset_delay);

            /* Schedule the buffer for playback */
            scheduler_schedule_event(astrid_scheduler, buf, msg.onset_delay);

            /* If the buffer is flagged to loop, schedule the next render 
             * by placing the message onto the message scheduling priority 
             * queue with a timestamp for sending the render message 50% 
             * into the buffer playback, with an onset delay for the buffer 
             * making up the remaining 50%. TODO: measure jitter when scheduling 
             * in regular intervals. */

            syslog(LOG_INFO, "Finished scheduling rendered event for playback from message\n");
            syslog(LOG_INFO, "\ninitiated %f\nscheduled %f\ncompleted %f\nmax_processing_time %f\nonset_delay %ld\nvoice_id %ld\ncount %ld\ntype %d\nmsg %s\nname %s\n\n", 
                msg.initiated,
                msg.scheduled,
                msg.completed,
                msg.max_processing_time,
                msg.onset_delay,
                msg.voice_id, 
                msg.count,
                msg.type,
                msg.msg,
                msg.instrument_name
            );


            if(buf->is_looping == 1) {
                msg.initiated = now;
                msg.scheduled = (lpfloat_t)buf->length / ASTRID_SAMPLERATE;
                msg.type = LPMSG_PLAY;
                msg.count += 1;
                //msg.max_processing_time = msg.scheduled; /* This schedules the render as soon as possible */
                msg.max_processing_time = 0;

                syslog(LOG_INFO, "Scheduling message for loop retriggering\n");
                syslog(LOG_INFO, "\ninitiated %f\nscheduled %f\ncompleted %f\nmax_processing_time %f\nonset_delay %ld\nvoice_id %ld\ncount %ld\ntype %d\nmsg %s\nname %s\n\n", 
                    msg.initiated,
                    msg.scheduled,
                    msg.completed,
                    msg.max_processing_time,
                    msg.onset_delay,
                    msg.voice_id, 
                    msg.count,
                    msg.type,
                    msg.msg,
                    msg.instrument_name
                );

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
int audio_callback(jack_nframes_t nframes, __attribute__((unused)) void * arg) {
    jack_nframes_t i;
    jack_default_audio_sample_t * out1, * out2;

    out1 = (jack_default_audio_sample_t *)jack_port_get_buffer(astrid_outport1, nframes);
    out2 = (jack_default_audio_sample_t *)jack_port_get_buffer(astrid_outport2, nframes);

    for(i=0; i < nframes; i++) {
        lpscheduler_tick(astrid_scheduler);
        out1[i] = astrid_scheduler->current_frame[0];
        out2[i] = astrid_scheduler->current_frame[1];
    }

    return 0;
}

int cleanup(
    pthread_t buffer_feed_thread, 
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
    jack_client_close(jack_client);

    syslog(LOG_INFO, "Cleaning up scheduler...\n");
    if(astrid_scheduler != NULL) scheduler_destroy(astrid_scheduler);

    syslog(LOG_INFO, "Closing sessiondb...\n");
    if(sessiondb != NULL) lpsessiondb_close(sessiondb);

    syslog(LOG_INFO, "Done with cleanup!\n");

    closelog();

    return 0;
}

int main() {
    struct sigaction shutdown_action;
    lpcounter_t voice_id_counter;
    pthread_t buffer_feed_thread;
    jack_status_t jack_status;
    jack_options_t jack_options = JackNullOption;

    sessiondb = NULL;
    openlog("astrid-dac", LOG_PID, LOG_USER);

    /* Set shutdown signal handlers */
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = SA_RESTART; /* Prevent open, read, write etc from EINTR */

    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGINT signal handler. Error: %s\n", strerror(errno));
        exit(1);
    }

    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "Could not init SIGTERM signal handler. Error: %s\n", strerror(errno));
        exit(1);
    }

    /* init scheduler
     * 
     * The scheduler is shared between the miniaudio callback 
     * and astrid buffer feed threads. The buffer feed thread 
     * may schedule buffers by adding them to the internal linked 
     * list in the scheduler. The miniaudio callback may read from 
     * the linked list, increment counts in playing buffers and 
     * flag buffers as having playback completed. 
     **/
    astrid_scheduler = scheduler_create(1, ASTRID_CHANNELS, ASTRID_SAMPLERATE);

    /* Set up shared memory IPC for voice IDs */
    if(lpcounter_create(&voice_id_counter) < 0) {
        syslog(LOG_ERR, "Could not initialize voice ID shared memory. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

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

    /* Start buffer feed thread */
    if(pthread_create(&buffer_feed_thread, NULL, buffer_feed, NULL) != 0) {
        syslog(LOG_ERR, "Could not initialize buffer feed thread. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Set up JACK */
    jack_client = jack_client_open("astrid-dac", jack_options, &jack_status, NULL);
    print_jack_status(jack_status);
    if(jack_client == NULL) {
        syslog(LOG_ERR, "Could not open jack client. Client is NULL: %s\n", strerror(errno));

        if((jack_status & JackServerFailed) == JackServerFailed) {
            syslog(LOG_ERR, "Could not open jack client. Jack server failed with status %2.0x\n", jack_status);
        } else {
            syslog(LOG_ERR, "Could not open jack client. Unknown error: %s\n", strerror(errno));
        }
        goto exit_with_error;
    }

    if((jack_status & JackServerStarted) == JackServerStarted) {
        syslog(LOG_INFO, "Jack server started!\n");
        goto exit_with_error;
    }

    jack_set_process_callback(jack_client, audio_callback, NULL);
    astrid_outport1 = jack_port_register(jack_client, "outL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    astrid_outport2 = jack_port_register(jack_client, "outR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if((astrid_outport1 == NULL) || (astrid_outport2 == NULL)) {
        syslog(LOG_ERR, "No more JACK ports available, shutting down...\n");
        goto exit_with_error;
    }

    if(jack_activate(jack_client) != 0) {
        syslog(LOG_ERR, "Could not activate JACK client, shutting down...\n");
        goto exit_with_error;
    }

    syslog(LOG_INFO, "Astrid DAC is starting...\n");
    while(astrid_is_running) {
        /* Twiddle thumbs & tidy up */
        usleep((useconds_t)10000);
        scheduler_cleanup_nursery(astrid_scheduler);
    }

    return cleanup(buffer_feed_thread, sessiondb);

exit_with_error:
    cleanup(buffer_feed_thread, sessiondb);
    syslog(LOG_ERR, "Exited with error\n");
    return 1;
}


