#include <hiredis/hiredis.h>

#include "pippi.h"
#include "astrid.h"

char * serialize_buffer(lpbuffer_t * buf, char * instrument_name) {
    size_t strsize, audiosize, offset, namesize;
    char * str;

    audiosize = buf->length * buf->channels * sizeof(lpfloat_t);
    namesize = strlen(instrument_name);

    strsize =  0;
    strsize += sizeof(size_t); /* length     */
    strsize += sizeof(size_t); /* namelen    */
    strsize += sizeof(int);    /* channels   */
    strsize += sizeof(int);    /* samplerate */
    strsize += sizeof(int);    /* is_looping */
    strsize += sizeof(size_t); /* onset      */
    strsize += audiosize;      /* audio data */

    /* initialize string buffer */
    str = calloc(1, strsize);

    offset = 0;

    memcpy(str + offset, &audiosize, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(str + offset, &namesize, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(str + offset, &buf->length, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(str + offset, &buf->channels, sizeof(int));
    offset += sizeof(int);

    memcpy(str + offset, &buf->samplerate, sizeof(int));
    offset += sizeof(int);

    memcpy(str + offset, &buf->is_looping, sizeof(int));
    offset += sizeof(int);

    memcpy(str + offset, &buf->onset, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(str + offset, buf->data, audiosize);
    offset += audiosize;

    memcpy(str + offset, instrument_name, namesize);

    return str;
}

lpbuffer_t * deserialize_buffer(char * str, char ** name) {
    size_t audiosize, offset, length, onset, namesize;
    int channels, samplerate, is_looping;
    char * _name;
    lpbuffer_t * buf;
    lpfloat_t * audio;

    offset = 0;

    memcpy(&audiosize, str + offset, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(&namesize, str + offset, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(&length, str + offset, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(&channels, str + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&samplerate, str + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&is_looping, str + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&onset, str + offset, sizeof(size_t));
    offset += sizeof(size_t);

    audio = calloc(1, audiosize);
    memcpy(audio, str + offset, audiosize);
    offset += audiosize;

    _name = calloc(namesize+1, sizeof(char));
    memcpy(_name, str + offset, namesize+1);
    *name = _name;

    buf = calloc(1, sizeof(lpbuffer_t));

    buf->length = length;
    buf->channels = channels;
    buf->samplerate = samplerate;
    buf->is_looping = is_looping;
    buf->data = audio;
    buf->onset = onset;

    buf->phase = 0.f;
    buf->pos = 0;
    buf->boundry = length-1;
    buf->range = length;

    return buf;
}

void send_play_message(char * instrument_name) {
    redisContext * redis_ctx;
    redisReply * redis_reply;
    ssize_t cmd_size;
    char * play_cmd;
    struct timeval redis_timeout = {15, 0};

    /* FIXME pass in a connection instead */
    redis_ctx = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    if(redis_ctx == NULL) {
        fprintf(stderr, "Could not start connection to redis.\n");
        exit(1);
    }

    if(redis_ctx->err) {
        fprintf(stderr, "There was a problem while connecting to redis. %s\n", redis_ctx->errstr);
        exit(1);
    }

    cmd_size = snprintf(NULL, 0, "LPUSH astrid-play-%s p", instrument_name) + 1;
    play_cmd = calloc(cmd_size, sizeof(char));
    snprintf(play_cmd, cmd_size, "LPUSH astrid-play-%s p", instrument_name);

    redis_reply = redisCommand(redis_ctx, play_cmd);
    if(redis_reply->str != NULL) printf("play result: %s\n", redis_reply->str); 
    freeReplyObject(redis_reply);
    free(instrument_name);

    if(redis_ctx != NULL) redisFree(redis_ctx); 
}


