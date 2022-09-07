#include <hiredis/hiredis.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pippi.h"
#include "astrid.h"

size_t lpadc_get_pos(lpadcbuf_t * adcbuf) {
    size_t pos;
    pos = 0;
    memcpy(&pos, adcbuf->buf, sizeof(size_t));
    return pos;
}

void lpadc_set_pos(lpadcbuf_t * adcbuf, size_t pos) {
    memcpy(adcbuf->buf, &pos, sizeof(size_t));
}

size_t lpadc_get_length(lpadcbuf_t * adcbuf) {
    size_t length;
    length = 0;
    memcpy(&length, adcbuf->buf + sizeof(size_t), sizeof(size_t));
    return length;
}

void lpadc_set_length(lpadcbuf_t * adcbuf, size_t length) {
    memcpy(adcbuf->buf + sizeof(size_t), &length, sizeof(size_t));
}

size_t lpadc_get_channels(lpadcbuf_t * adcbuf) {
    int channels;
    channels = 0;
    memcpy(&channels, adcbuf->buf + (sizeof(size_t)*2), sizeof(int));
    return channels;
}

void lpadc_set_channels(lpadcbuf_t * adcbuf, int channels) {
    memcpy(adcbuf->buf + (sizeof(size_t)*2), &channels, sizeof(int));
}

lpadcbuf_t * lpadc_open_for_writing() {
    lpadcbuf_t * adcbuf;
    size_t bufoffset;
    int bufinit;
    struct stat buffdstat;

    adcbuf = (lpadcbuf_t *)calloc(sizeof(lpadcbuf_t), 1);
    adcbuf->fd = shm_open(LPADC_BUFNAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(adcbuf->fd == -1) {
        fprintf(stderr, "Could not get a handle to adcbuf shared memory.\n");
        exit(1);
    }

    /* FIXME get samplerate & channels from context */
    adcbuf->frames = (size_t)(ASTRID_ADCSECONDS * ASTRID_SAMPLERATE);
    adcbuf->channels = ASTRID_CHANNELS;
    adcbuf->headsize = sizeof(size_t) + sizeof(size_t) + sizeof(int);
    adcbuf->fullsize = adcbuf->headsize + (sizeof(lpfloat_t) * adcbuf->frames * adcbuf->channels);
    adcbuf->pos = 0;

    /* fstat check size */
    if(fstat(adcbuf->fd, &buffdstat) == -1) {
        fprintf(stderr, "Could not fstat adcbuf shared memory.\n");
        exit(1);
    }

    bufinit = 0;
    /* if less than bufsize bytes, resize and init: */
    if((size_t)buffdstat.st_size < adcbuf->fullsize) {
        if(ftruncate(adcbuf->fd, adcbuf->fullsize) == -1) {
            fprintf(stderr, "Could not resize adcbuf.\n");
            exit(1);
        }

        bufinit = 1;
    }

    adcbuf->buf = mmap(NULL, adcbuf->fullsize, PROT_READ | PROT_WRITE, MAP_SHARED, adcbuf->fd, 0);
    if(adcbuf->buf == MAP_FAILED) {
        fprintf(stderr, "Could not mmap adcbuf.\n");
        exit(1);
    }

    /* If we just created the adcbuf, init the header */
    if(bufinit == 1) {
        bufoffset = 0;

        memcpy(adcbuf->buf, &adcbuf->pos, sizeof(size_t));
        bufoffset += sizeof(size_t);

        memcpy(adcbuf->buf+bufoffset, &adcbuf->frames, sizeof(size_t));
        bufoffset += sizeof(size_t);

        memcpy(adcbuf->buf+bufoffset, &adcbuf->channels, sizeof(int));
    }

    return adcbuf;
}

lpadcbuf_t * lpadc_open_for_reading() {
    lpadcbuf_t * adcbuf;
    struct stat buffdstat;

    adcbuf = (lpadcbuf_t *)calloc(sizeof(lpadcbuf_t), 1);
    adcbuf->fd = shm_open(LPADC_BUFNAME, O_RDONLY, 0);
    if(adcbuf->fd == -1) {
        fprintf(stderr, "Could not get a handle to adcbuf shared memory.\n");
        exit(1);
    }

    /* fstat check size */
    if(fstat(adcbuf->fd, &buffdstat) == -1) {
        fprintf(stderr, "Could not fstat adcbuf shared memory.\n");
        exit(1);
    }

    /* FIXME get samplerate & channels from context */
    adcbuf->headsize = sizeof(size_t) + sizeof(size_t) + sizeof(int);
    adcbuf->fullsize = buffdstat.st_size;

    adcbuf->buf = mmap(NULL, adcbuf->fullsize, PROT_READ, MAP_SHARED, adcbuf->fd, 0);
    if(adcbuf->buf == MAP_FAILED) {
        fprintf(stderr, "Could not mmap adcbuf.\n");
        exit(1);
    }

    adcbuf->pos = lpadc_get_pos(adcbuf);
    adcbuf->frames = lpadc_get_length(adcbuf);
    adcbuf->channels = lpadc_get_channels(adcbuf);

    return adcbuf;
}

lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, int channel) {
    lpfloat_t sample;
    size_t offset;
    int channels;

    // get buf channels
    channels = lpadc_get_channels(adcbuf);

    // get offset to buf
    offset = adcbuf->headsize;
    offset += ((adcbuf->pos * channels + channel) * sizeof(lpfloat_t));

    // memcpy at offset to lpfloat_t
    sample = 0;
    memcpy(&sample, adcbuf+offset, sizeof(lpfloat_t));

    return sample;
}

size_t lpadc_increment_pos(lpadcbuf_t * adcbuf) {
    size_t pos, length;

    pos = lpadc_get_pos(adcbuf) + 1;
    length = lpadc_get_length(adcbuf);
    pos = pos % length;

    lpadc_set_pos(adcbuf, pos);

    return pos;
}

size_t lpadc_write_sample(lpadcbuf_t * adcbuf, lpfloat_t sample, int channel, ssize_t offset) {
    size_t pos, writepos;
    int channels;

    pos = lpadc_get_pos(adcbuf);
    channels = lpadc_get_channels(adcbuf);
    writepos = adcbuf->headsize + ((((pos + offset) * channels) + channel) * sizeof(lpfloat_t));
    memcpy(adcbuf->buf + writepos, &sample, sizeof(lpfloat_t));

    return pos;
}


void lpadc_close(lpadcbuf_t * adcbuf) {
    if(close(adcbuf->fd) == -1) {
        fprintf(stderr, "Could not close adcbuf fd.\n");
        exit(1);
    }
}

void lpadc_destroy(lpadcbuf_t * adcbuf) {
    if(shm_unlink(LPADC_BUFNAME) == -1) {
        fprintf(stderr, "Could not unlink adcbuf fd.\n");
        exit(1);
    }
    free(adcbuf);
}


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


