#ifndef LPASTRID_H
#define LPASTRID_H

#define NUM_RENDERERS 10
#define ASTRID_CHANNELS 2
#define ASTRID_SAMPLERATE 48000
#define ASTRID_ADCSECONDS 30

#define PLAY_MESSAGE "p"
#define STOP_MESSAGE "s"
#define SHUTDOWN_MESSAGE "k"

#define LPADC_BUFNAME "/lpadcbuf"

char * serialize_buffer(lpbuffer_t * buf, char * instrument_name); 
lpbuffer_t * deserialize_buffer(char * str, char ** name); 
void send_play_message(char * instrument_name);

typedef struct lpadcbuf_t {
    int fd;
    char * buf;
    size_t headsize;
    size_t fullsize;
    size_t pos;
    size_t frames;
    int channels;
} lpadcbuf_t;

lpadcbuf_t * lpadc_open_for_writing();
lpadcbuf_t * lpadc_open_for_reading();
lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, int channel);
size_t lpadc_write_sample(lpadcbuf_t * adcbuf, lpfloat_t sample, int channel, ssize_t offset);
size_t lpadc_increment_pos(lpadcbuf_t * adcbuf);
size_t lpadc_get_pos(lpadcbuf_t * adcbuf);
void lpadc_close(lpadcbuf_t * adcbuf);
void lpadc_destroy(lpadcbuf_t * adcbuf);

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
