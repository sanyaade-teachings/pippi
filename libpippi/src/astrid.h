#ifndef LPASTRID_H
#define LPASTRID_H

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
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
#define ASTRID_ADCSECONDS 30

#define SAMPLER_NAME_SIZE 15
#define SAMPLER_NAME_TEMPLATE "/lpsampler%04d"

#define PLAY_MESSAGE "p"
#define STOP_MESSAGE "s"
#define SHUTDOWN_MESSAGE "k"

#define SPACE ' '
#define LPMAXNAME 20
#define LPMAXMSG (PIPE_BUF - sizeof(size_t) - sizeof(size_t))
#define LPADC_BUFNAME "/lpadcbuf"
#define LPPLAYQ "/tmp/astridq"
#define LPMAXQNAME (12 + 1 + LPMAXNAME)

typedef struct lpdacctx_t {
    lpscheduler_t * s;
    int channels;
    float samplerate;
} lpdacctx_t;

typedef struct lpeventctx_t {
    char instrument_name[LPMAXNAME];
    char play_params[LPMAXMSG];
} lpeventctx_t;

typedef struct lpmsg_t {
    size_t timestamp;
    size_t voice_id;
    char msg[LPMAXMSG];
} lpmsg_t;

char * serialize_buffer(lpbuffer_t * buf, char * instrument_name, char * play_params); 
lpbuffer_t * deserialize_buffer(char * str, lpeventctx_t * ctx); 
int send_play_message(lpeventctx_t * ctx);
int get_play_message(char * instrument_name, lpmsg_t * msg);

int astrid_playq_open(char * instrument_name);
int astrid_playq_read(int qfd, lpmsg_t * msg);
int astrid_playq_close(int qfd);

typedef struct lpsampler_t {
    int fd;
    char * name;
    char * buf;
    size_t total_bytes;
    size_t length;
    int samplerate;
    int channels;

    size_t length_offset;
    size_t samplerate_offset;
    size_t channels_offset;
    size_t buffer_offset;
} lpsampler_t;

lpsampler_t * lpsampler_create_from(int bankid, lpbuffer_t * buf);
lpsampler_t * lpsampler_open(int bankid);
lpsampler_t * lpsampler_dub(int bankid, lpbuffer_t * buf);
int lpsampler_close(lpsampler_t *);
int lpsampler_destroy(lpsampler_t *);
int lpsampler_get_length(lpsampler_t * sampler, size_t * length);
int lpsampler_get_samplerate(lpsampler_t * sampler, int * samplerate);
int lpsampler_get_channels(lpsampler_t * sampler, int * channels);
lpfloat_t lpsampler_read_sample(lpsampler_t * sampler, size_t frame, int channel);

typedef struct lpadcbuf_t {
    int fd;
    char * buf;
    size_t total_bytes;
    size_t pos;
    size_t length;
    int channels;

    size_t pos_offset;
    size_t length_offset;
    size_t channels_offset;
    size_t buffer_offset;
} lpadcbuf_t;

lpadcbuf_t * lpadc_create();
lpadcbuf_t * lpadc_open();
lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, size_t frame, int channel);
size_t lpadc_write_sample(lpadcbuf_t * adcbuf, lpfloat_t sample, size_t frame, int channel, ssize_t offset);
int lpadc_increment_pos(lpadcbuf_t * adcbuf, int count);
int lpadc_get_pos(lpadcbuf_t * adcbuf, size_t * pos);
int lpadc_close(lpadcbuf_t * adcbuf);
int lpadc_destroy(lpadcbuf_t * adcbuf);

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

#endif
