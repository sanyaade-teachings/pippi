#ifndef LPASTRID_H
#define LPASTRID_H

#include <stdatomic.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
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
#include <termios.h>
#include <unistd.h>

#include "pippi.h"

#include "lmdb.h"
#include "midl.h"
#include "pqueue.h"
#include <hiredis/hiredis.h>
#include <jack/jack.h>

#define NUM_NODES 4096
#define NUM_RENDERERS 10
#define ASTRID_CHANNELS 2
#define ASTRID_SAMPLERATE 48000

#define TOKEN_PROJECT_ID 'x'
#define LPIPC_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

#define ASTRID_MQ_MAXMSG 10

/* queue paths */
#define LPPLAYQ "/astridq"
#define ASTRID_MSGQ_PATH "/astrid-msgq"
#define LPMAXQNAME (12 + 1 + LPMAXNAME)

#define ASTRID_SESSIONDB_PATH "/tmp/astrid_session.db"
#define ASTRID_MIDI_TRIGGERQ_PATH "/tmp/astrid-miditriggerq"
#define ASTRID_MIDI_CCBASE_PATH "/tmp/astrid-mididevice%d-cc%d"
#define ASTRID_MIDI_NOTEBASE_PATH "/tmp/astrid-mididevice%d-note%d"
#define ASTRID_MIDIMAP_NOTEBASE_PATH "/tmp/astrid-midimap-device%d-note%d"
#define ASTRID_IPC_IDBASE_PATH "/tmp/astrid-idfile-%s"

#define ASTRID_SERIAL_CTLBASE_PATH "/tmp/astrid-serialdevice%d-ctl%d"

#define LPKEY_MAXLENGTH 4096

#define PLAY_MESSAGE 'p'
#define UPDATE_MESSAGE 'u'
#define TRIGGER_MESSAGE 't'
#define SERIAL_MESSAGE 'b'
#define SCHEDULE_MESSAGE 's'
#define LOAD_MESSAGE 'l'
#define SHUTDOWN_MESSAGE 'q'
#define SET_COUNTER_MESSAGE 'v'

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

#define ASTRID_ADCSECONDS 10
#define LPADCBUFFRAMES (ASTRID_SAMPLERATE * ASTRID_ADCSECONDS)
#define LPADCBUFSAMPLES (LPADCBUFFRAMES * ASTRID_CHANNELS)
#define LPADCBUFBYTES (LPADCBUFSAMPLES * sizeof(lpfloat_t))
#define LPADC_WRITEPOS_PATH "/tmp/astrid-adc-writepos"
#define LPADC_BUFFER_PATH "/astrid-adc-buffer"

#define ASTRID_DEVICEID_PATH "/tmp/astrid_device_id"

typedef struct lpcounter_t {
    int shmid;
    int semid;
} lpcounter_t;

typedef struct lpmsgpq_node_t {
    double timestamp;
    lpmsg_t msg;
    size_t pos;
    int index; /* index in node pool */
} lpmsgpq_node_t;


/* When instrument scripts produce MIDI triggers, 
 * they schedule them (with the onset value, which 
 * is relative to *now*) by sending this struct over 
 * the MIDI trigger fifo. */
typedef struct lpmidievent_t {
    double onset;
    double now;
    double length;
    char type;
    char note;
    char velocity;
    char program;
    char bank_msb;
    char bank_lsb;
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

typedef struct lpinstrument_t {
    const char * name;
    int channels;
    volatile int is_running;
    volatile int is_waiting;
    int has_been_initialized;
    lpfloat_t samplerate;

    MDB_cursor * dbcur;
    MDB_env * dbenv;
    MDB_dbi dbi;
    MDB_txn * dbtxn_read;
    MDB_txn * dbtxn_write;

    mqd_t playqd;
    lpmsg_t msg;

    pqueue_t * msgpq;
    lpmsgpq_node_t * pqnodes;

    pthread_t message_feed_thread;
    pthread_t message_scheduler_pq_thread;
    pthread_t buffer_feed_thread;
    lpscheduler_t * async_mixer;
    lpbuffer_t * lastbuf;

    jack_port_t ** inports;
    jack_port_t ** outports;
    jack_client_t * jack_client;

    // Optional local context struct for callbacks
    void * context;

    // Param update callback
    void (*updates)(void * instrument);

    // Async renderer callback (C-compat only)
    lpbuffer_t * (*renderer)(void * instrument);

    // Stream callback
    void (*stream)(int channels, size_t blocksize, float ** input, float ** output, void * instrument);

    // Shutdown signal handler (SIGTERM & SIGKILL)
    void (*shutdown)(int sig);
} lpinstrument_t;


void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay);
void lpscheduler_tick(lpscheduler_t * s);
lpscheduler_t * scheduler_create(int, int, lpfloat_t);
void scheduler_destroy(lpscheduler_t * s);
int lpscheduler_get_now_seconds(double * now);
void scheduler_cleanup_nursery(lpscheduler_t * s);

int lpcounter_create(lpcounter_t * c);
ssize_t lpcounter_read_and_increment(lpcounter_t * c);
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

int parse_message_from_args(int argc, int arg_offset, char * argv[], lpmsg_t * msg);

ssize_t astrid_get_voice_id();

int send_message(lpmsg_t msg);
int send_serial_message(lpmsg_t msg);
int send_play_message(lpmsg_t msg);
int get_play_message(char * instrument_name, lpmsg_t * msg);

mqd_t astrid_playq_open(const char * instrument_name);
int astrid_playq_read(mqd_t mqd, lpmsg_t * msg);
int astrid_playq_close(mqd_t mqd);

mqd_t astrid_msgq_open();
int astrid_msgq_close(mqd_t mqd);
int astrid_msgq_read(mqd_t mqd, lpmsg_t * msg);


/* TODO add POSIX message queues for these too */
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

int lpserial_setctl(int device_id, int param_id, size_t value);
int lpserial_getctl(int device_id, int ctl, lpfloat_t * value);

int astrid_get_playback_device_id();
int astrid_get_capture_device_id();

int lpadc_create();
int lpadc_destroy();
int lpadc_aquire(lpipc_buffer_t ** adcbuf);
int lpadc_increment_and_release(lpipc_buffer_t * adcbuf, size_t increment_in_samples);
int lpadc_write_block(const void * block, size_t blocksize);
int lpadc_read_sample(size_t offset, lpfloat_t * sample);
/*int lpadc_read_block_of_samples(size_t offset, size_t size, lpfloat_t (*out)[LPADCBUFSAMPLES]);*/
int lpadc_read_block_of_samples(size_t offset, size_t size, lpfloat_t * out);

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

int astrid_instrument_start(const char * name, int channels, void * ctx, lpinstrument_t * instrument, int argc, char ** argv);
int astrid_instrument_stop(lpinstrument_t * instrument);

void astrid_instrument_set_param_float(lpinstrument_t * instrument, int param_index, lpfloat_t value);
lpfloat_t astrid_instrument_get_param_float(lpinstrument_t * instrument, int param_index, lpfloat_t default_value);
void astrid_instrument_set_param_float_list(lpinstrument_t * instrument, int param_index, lpfloat_t * value, size_t size);
void astrid_instrument_get_param_float_list(lpinstrument_t * instrument, int param_index, size_t size, lpfloat_t * list);
lpfloat_t astrid_instrument_get_param_float_list_item(lpinstrument_t * instrument, int param_index, size_t size, int item_index, lpfloat_t default_value);
int astrid_instrument_tick(lpinstrument_t * instrument);
int astrid_instrument_session_open(lpinstrument_t * instrument);
int astrid_instrument_session_close(lpinstrument_t * instrument);

int lpencode_with_prefix(char * prefix, size_t val, char * encoded);
size_t lpdecode_with_prefix(char * encoded);


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
