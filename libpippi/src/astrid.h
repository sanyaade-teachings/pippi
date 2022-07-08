#ifndef LPASTRID_H
#define LPASTRID_H

#define NUM_RENDERERS 10
#define ASTRID_CHANNELS 2
#define ASTRID_SAMPLERATE 48000

#define PLAY_MESSAGE "p"
#define SHUTDOWN_MESSAGE "s"

char * serialize_buffer(lpbuffer_t * buf); 
lpbuffer_t * deserialize_buffer(char * str); 
void send_play_message();

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
