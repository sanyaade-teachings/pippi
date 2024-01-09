#include "astrid.h"
#include "pqueue.h"


#define NUM_NODES 4096

static volatile int astrid_is_running = 1;
int pqnode_index = 0;
pqueue_t * msgpq;
lpmsgpq_node_t * pqnodes;

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

/* Message scheduler priority queue comparison callbacks */
static int msgpq_cmp_pri(double next, double curr) {
    return (next > curr);
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

int msgpq_remove_nodes_by_voice_id(size_t voice_id) {
    lpmsgpq_node_t * node;
    lpmsg_t * msg;
    void * d;
    size_t i;

    for(i=0; i < msgpq->size; i++) {
        d = msgpq->d[i];                
        if(d == NULL) {
            syslog(LOG_DEBUG, "STOP: d[%d] is NULL\n", (int)i);
            continue;
        }

        node = (lpmsgpq_node_t *)d;
        msg = &node->msg;
        if(msg->voice_id == voice_id) {
            syslog(LOG_DEBUG, "STOP: removing node & freeing memory for voice id %ld\n", voice_id);
            pqueue_remove(msgpq, d);
        }
    }

    return 0;
}

/* Message scheduler priority queue thread handler */
void * message_scheduler_pq(__attribute__((unused)) void * arg) {
    lpmsg_t * msg;
    lpmsgpq_node_t * node;
    void * d;
    double now;

    now = 0;
    syslog(LOG_DEBUG, " MPQ           STARTING\n");

    d = NULL;
    msg = NULL;
    node = NULL;

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
        msg = &node->msg;

        if(msg->type == LPMSG_SHUTDOWN) {
            break;
        }

        /* Get now */
        if(lpscheduler_get_now_seconds(&now) < 0) {
            syslog(LOG_CRIT, "Error getting now in message scheduler\n");
            exit(1);
        }

        /* if this is a STOP_VOICE message, find all voice events and remove them */
        if(msg->type == LPMSG_STOP_VOICE) {
            if(msgpq_remove_nodes_by_voice_id(msg->voice_id) < 0) {
                syslog(LOG_ERR, "Error removing voice %ld nodes from priority queue\n", msg->voice_id);
                usleep((useconds_t)500);
                continue;
            }

            syslog(LOG_INFO, "Got STOP_VOICE message... removed voice %ld nodes from pq\n", msg->voice_id);
            continue;
        }

        /* If this is a STOP_INSTRUMENT message, find all instrument events and remove them */
        if(msg->type == LPMSG_STOP_INSTRUMENT) {
            syslog(LOG_INFO, "Got STOP_INSTRUMENT message... ignoring it\n");
            continue;
        }

        /* If msg timestamp is in the future, 
         * sleep for a bit and then try again */
        if(node->timestamp > now) {
            usleep((useconds_t)500);
            continue;
        }

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
    }

    syslog(LOG_INFO, "Message scheduler pq thread shutting down...\n");
    pqueue_free(msgpq);
    free(pqnodes);
    return 0;
}

void * message_feed(__attribute__((unused)) void * arg) {
    double seq_delay;
    mqd_t qd;
    lpmsg_t msg = {0};
    lpmsgpq_node_t * d;
    double now = 0;

    if((qd = astrid_msgq_open()) == (mqd_t) -1) {
        syslog(LOG_CRIT, "Could not open msgq for message relay: %s\n", strerror(errno));
        exit(1);        
    }

    syslog(LOG_INFO, "Message relay: Waiting for messages...\n");

    /* Wait for messages on the msgq fifo */
    while(astrid_is_running) {
        if(astrid_msgq_read(qd, &msg) < 0) {
            syslog(LOG_ERR, "Error while fetching message during message relay loop: %s\n", strerror(errno));
            continue;
        }

        d = &pqnodes[pqnode_index];
        memcpy(&d->msg, &msg, sizeof(lpmsg_t));
        pqnode_index += 1;
        while(pqnode_index >= NUM_NODES) pqnode_index -= NUM_NODES;

        if(lpscheduler_get_now_seconds(&now) < 0) {
            syslog(LOG_CRIT, "Error getting now in seq relay\n");
            exit(1);
        }

        /* Hold on to the message as long as possible while still 
         * trying to leave some time for processing before the target deadline */
        seq_delay = msg.scheduled - (msg.max_processing_time * 2);
        d->timestamp = msg.initiated + seq_delay;

        if(pqueue_insert(msgpq, (void *)d) < 0) {
            syslog(LOG_ERR, "Error while inserting message into pq during msgq loop: %s\n", strerror(errno));
            continue;
        }

        /* Exit the loop on shutdown message after sending 
         * it along to the priority queue as well */
        if(msg.type == LPMSG_SHUTDOWN) break;
    }

    syslog(LOG_INFO, "lpmsg_t relay: Message relay shutting down...\n");
    if(qd != (mqd_t) -1) astrid_msgq_close(qd);

    return 0;
}


int cleanup(pthread_t message_feed_thread, pthread_t message_scheduler_pq_thread) {
    astrid_is_running = 0;

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
    pthread_t message_feed_thread;
    pthread_t message_scheduler_pq_thread;

    int i = 0;

    openlog("astrid-seq", LOG_PID, LOG_USER);

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

    /* Allocate the pq message nodes */
    if((pqnodes = (lpmsgpq_node_t *)calloc(NUM_NODES, sizeof(lpmsgpq_node_t))) == NULL) {
        syslog(LOG_ERR, "Could not initialize message priority queue nodes. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    for(i=0; i < NUM_NODES; i++) {
        pqnodes[i].index = i;
    }

    /* Create the message priority queue */
    if((msgpq = pqueue_init(NUM_NODES, msgpq_cmp_pri, msgpq_get_pri, msgpq_set_pri, msgpq_get_pos, msgpq_set_pos)) == NULL) {
        syslog(LOG_ERR, "Could not initialize message priority queue. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Start message feed thread */
    if(pthread_create(&message_feed_thread, NULL, message_feed, NULL) != 0) {
        syslog(LOG_ERR, "Could not initialize message feed thread. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    /* Start message pq thread */
    if(pthread_create(&message_scheduler_pq_thread, NULL, message_scheduler_pq, NULL) != 0) {
        syslog(LOG_ERR, "Could not initialize message scheduler pq thread. Error: %s\n", strerror(errno));
        goto exit_with_error;
    }

    syslog(LOG_INFO, "Astrid SEQ is starting...\n");
    while(astrid_is_running) {
        /* Twiddle thumbs */
        usleep((useconds_t)1000);
    }

    return cleanup(message_feed_thread, message_scheduler_pq_thread);

exit_with_error:
    cleanup(message_feed_thread, message_scheduler_pq_thread);
    syslog(LOG_ERR, "Exited with error\n");
    return 1;
}


