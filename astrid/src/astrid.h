#ifndef LPASTRID_H
#define LPASTRID_H

#include <stdatomic.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/syscall.h>
#include <semaphore.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "pippi.h"

#define NUM_RENDERERS 10
#define ASTRID_CHANNELS 2
#define ASTRID_SAMPLERATE 48000

#define TOKEN_PROJECT_ID 'x'
#define LPIPC_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

#define ASTRID_SESSIONDB_PATH "/tmp/astrid_session.db"
#define ASTRID_MSGQ_PATH "/tmp/astrid-msgq"
#define ASTRID_MIDI_TRIGGERQ_PATH "/tmp/astrid-miditriggerq"
#define ASTRID_MIDI_CCBASE_PATH "/tmp/astrid-mididevice%d-cc%d"
#define ASTRID_MIDI_NOTEBASE_PATH "/tmp/astrid-mididevice%d-note%d"
#define ASTRID_MIDIMAP_NOTEBASE_PATH "/tmp/astrid-midimap-device%d-note%d"

#define PLAY_MESSAGE 'p'
#define TRIGGER_MESSAGE 't'
#define STOP_MESSAGE 's'
#define LOAD_MESSAGE 'l'
#define SHUTDOWN_MESSAGE 'k'

#ifndef NOTE_ON
#define NOTE_ON 144
#endif

#ifndef NOTE_OFF
#define NOTE_OFF 128
#endif

#ifndef CONTROL_CHANGE
#define CONTROL_CHANGE 176
#endif

#define SPACE ' '
#define LPMAXNAME 24
#define LPMAXMSG (PIPE_BUF - sizeof(double) - (sizeof(size_t) * 3) - sizeof(uint16_t) - LPMAXNAME)

#define ASTRID_ADCSECONDS 10
#define LPADCBUFFRAMES (ASTRID_SAMPLERATE * ASTRID_ADCSECONDS)
#define LPADCBUFSAMPLES (LPADCBUFFRAMES * ASTRID_CHANNELS)
#define LPADC_WRITEPOS_PATH "/tmp/astrid-adc-writepos"
#define LPADC_BUFFER_PATH "/tmp/astrid-adc-buffer"

/* deprecated */
#define LPADC_HANDLE "/tmp/astrid_adcbuf_shmid"
#define LPADC_BUFNAME "/lpadcbuf"

#define LPPLAYQ "/tmp/astridq" /* the path *prefix* used for instrument play queues */
#define LPMAXQNAME (12 + 1 + LPMAXNAME)
#define LPMSG_MAX_PQ 4096 /* preallocated message priority queue size */

#define LPVOICE_ID_SHMID "/tmp/astrid_voice_id_shmid"
#define LPVOICE_ID_SEMID "/tmp/astrid_voice_id_semid"

#define ASTRID_DEVICEID_PATH "/tmp/astrid_device_id"

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

enum LPMessageTypes {
    LPMSG_EMPTY,
    LPMSG_PLAY,
    LPMSG_TRIGGER,
    LPMSG_STOP,
    LPMSG_LOAD,
    LPMSG_SHUTDOWN,
    NUM_LPMESSAGETYPES
};

typedef struct lpcounter_t {
    int shmid;
    int semid;
} lpcounter_t;

/* The order of the members of this struct matters, 
 * since it must fit into exactly PIPE_BUF bytes. 
 * The members are arranged to eliminate padding. */
typedef struct lpmsg_t {
    double timestamp; /* Used to schedule the message itself */
    size_t onset_delay;     /* Used to supply an onset delay interval for playback or triggering */
    size_t voice_id;
    size_t count;
    uint16_t type;
    char msg[LPMAXMSG];
    char instrument_name[LPMAXNAME];
} lpmsg_t;

typedef struct lpmsgpq_node_t {
    double timestamp;
    lpmsg_t * msg;
    size_t pos;
} lpmsgpq_node_t;


/* When instrument scripts produce MIDI triggers, 
 * they schedule them (with the onset value, which 
 * is relative to *now*) by sending this struct over 
 * the MIDI trigger fifo. */
typedef struct lpmidievent_t {
    double onset;
    double length;
    char type;
    char note;
    char velocity;
    char channel;
} lpmidievent_t;

/* These events are what is stored in the 
 * scheduler's linked lists where it tracks 
 * which buffers are queued, playing, and 
 * completed, and which have pending callbacks.
 * */
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

void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay);
void lpscheduler_tick(lpscheduler_t * s);
lpscheduler_t * scheduler_create(int, int, lpfloat_t);
void scheduler_destroy(lpscheduler_t * s);
int lpscheduler_get_now_seconds(double * now);

int lpcounter_create(lpcounter_t * c);
int lpcounter_read_and_increment(lpcounter_t * c);
int lpcounter_destroy(lpcounter_t * c);

typedef struct lpdacctx_t {
    lpscheduler_t * s;
    int channels;
    float samplerate;
} lpdacctx_t;

/* compatible with lpbuffer_t */
typedef struct lpipc_buffer_t {
    size_t length;
    int samplerate;
    int channels;

    lpfloat_t phase;
    size_t boundry;
    size_t range;
    size_t pos;
    size_t onset;
    int is_looping;

    lpfloat_t data[];
} lpipc_buffer_t;

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

int parse_message_from_args(int argc, int arg_offset, char * argv[], lpmsg_t * msg);
int send_message(lpmsg_t msg);
int astrid_msgq_open();
int astrid_msgq_close(int qfd);
int astrid_msgq_read(int qfd, lpmsg_t * msg);

int midi_triggerq_open();
int midi_triggerq_schedule(int qfd, lpmidievent_t t);
int midi_triggerq_close(int qfd);

int lpmidi_setcc(int device_id, int cc, int value);
int lpmidi_getcc(int device_id, int cc);
int lpmidi_setnote(int device_id, int note, int velocity);
int lpmidi_getnote(int device_id, int note);

int lpmidi_add_msg_to_notemap(int device_id, int note, lpmsg_t msg);
int lpmidi_remove_msg_from_notemap(int device_id, int note, int index);
int lpmidi_print_notemap(int device_id, int note);
int lpmidi_trigger_notemap(int device_id, int note);

int lpadc_create();
int lpadc_destroy();
int lpadc_write_block(const void * block, size_t blocksize);
int lpadc_read_sample(size_t offset, lpfloat_t * sample);
int lpadc_read_block_of_samples(size_t offset, size_t size, lpfloat_t ** out);

int lpipc_buffer_create(char * id_path, size_t length, int channels, int samplerate);
int lpipc_buffer_aquire(char * id_path, lpipc_buffer_t ** buf);
int lpipc_buffer_release(char * id_path);
int lpipc_buffer_tolpbuffer(lpipc_buffer_t * ipcbuf, lpbuffer_t ** buf);
int lpipc_buffer_destroy(char * id_path);
int lpipc_setid(char * path, int id); 
int lpipc_getid(char * path); 
int lpipc_createvalue(char * path, size_t size);
int lpipc_setvalue(char * path, void * value, size_t size);
int lpipc_getvalue(char * path, void ** value);
int lpipc_releasevalue(char * id_path);
int lpipc_destroyvalue(char * id_path);

void lptimeit_since(struct timespec * start);

#ifdef LPSESSIONDB
#include <sqlite3.h>
int lpsessiondb_create(sqlite3 ** db);
int lpsessiondb_open_for_writing(sqlite3 ** db);
int lpsessiondb_open_for_reading(sqlite3 ** db);
int lpsessiondb_close(sqlite3 * db);
int lpsessiondb_insert_voice(lpmsg_t msg);
int lpsessiondb_mark_voice_active(sqlite3 * db, int voice_id);
int lpsessiondb_increment_voice_render_count(sqlite3 * db, int voice_id, size_t count);
int lpsessiondb_mark_voice_stopped(sqlite3 * db, int voice_id, size_t count);
#endif


#endif
