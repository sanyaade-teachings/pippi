#ifndef LPASTRID_H
#define LPASTRID_H

#include <stdatomic.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <pthread.h>
#endif

#include "pippi.h"

#define NUM_RENDERERS 10
#define ASTRID_CHANNELS 2
#define ASTRID_SAMPLERATE 48000
#define ASTRID_ADCSECONDS 10
#define LPADCBUFFRAMES (ASTRID_SAMPLERATE * ASTRID_ADCSECONDS)
#define LPADCBUFSIZE (LPADCBUFFRAMES * sizeof(float) * ASTRID_CHANNELS)

#define PLAY_MESSAGE "p"
#define STOP_MESSAGE "s"
#define SHUTDOWN_MESSAGE "k"

#define SPACE ' '
#define LPMAXNAME 24
#define LPMAXMSG (PIPE_BUF - sizeof(size_t) - sizeof(size_t) - LPMAXNAME)
#define LPADC_HANDLE "/tmp/astrid_adcbuf_shmid"
#define LPADC_BUFNAME "/lpadcbuf"
#define LPPLAYQ "/tmp/astridq"
#define LPMAXQNAME (12 + 1 + LPMAXNAME)

/* This struct is required for historical reasons by POSIX to be defined 
 * for system V semaphores. Astrid uses them for voice ID assignment. */
union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
#if defined(__linux__)
    struct seminfo * __buf;
#endif
};

typedef struct lpmsg_t {
    size_t delay;
    size_t voice_id;
    char instrument_name[LPMAXNAME];
    char msg[LPMAXMSG];
} lpmsg_t;

typedef struct lpevent_t {
    size_t id;
    lpbuffer_t * buf;
    size_t pos;
    size_t onset;
    void * next;
    void (*callback)(lpmsg_t msg);
    lpmsg_t msg;
    size_t callback_onset;
    int callback_fired;
} lpevent_t;

typedef struct lpscheduler_t {
    lpfloat_t * current_frame;
    int channels;
    int realtime;
    lpfloat_t samplerate;
    struct timespec * init;
    struct timespec * now;
    size_t ticks;
    size_t tick_ns;
    size_t event_count;
    size_t numzeros;
    lpfloat_t last_sum;
    lpevent_t * waiting_queue_head;
    lpevent_t * playing_stack_head;
    lpevent_t * nursery_head;
} lpscheduler_t;

void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay, void (*callback)(lpmsg_t), lpmsg_t msg, size_t callback_delay);
void lpscheduler_tick(lpscheduler_t * s);
lpscheduler_t * scheduler_create(int, int, lpfloat_t);
void scheduler_destroy(lpscheduler_t * s);
void lpscheduler_handle_callbacks(lpscheduler_t * s);


typedef struct lpdacctx_t {
    lpscheduler_t * s;
    int channels;
    float samplerate;
} lpdacctx_t;

typedef struct lpadcbuf_t {
    atomic_size_t pos;
    char buf[LPADCBUFSIZE];
} lpadcbuf_t;

typedef struct lpastridctx_t {
    lpscheduler_t * s;
    int channels;
    float samplerate;
    long voice_id;
    pthread_t thread_id;
    int voice_index;
    int is_playing;
    int is_looping;
} lpastridctx_t;



char * serialize_buffer(lpbuffer_t * buf, lpmsg_t * msg); 
lpbuffer_t * deserialize_buffer(char * str, lpmsg_t * msg); 
int send_play_message(lpmsg_t msg);
int get_play_message(char * instrument_name, lpmsg_t * msg);

int astrid_playq_open(char * instrument_name);
int astrid_playq_read(int qfd, lpmsg_t * msg);
int astrid_playq_close(int qfd);

int lpadc_create();
int lpadc_destroy();
lpadcbuf_t * lpadc_open();
void lpadc_write_block(lpadcbuf_t * adcbuf, float * block, size_t blocksize);
lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, size_t offset);

void lptimeit_since(struct timespec * start);

#endif
