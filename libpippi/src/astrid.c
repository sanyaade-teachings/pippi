#include <hiredis/hiredis.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pippi.h"
#include "astrid.h"

int lpadc_get_pos(lpadcbuf_t * adcbuf, size_t * pos) {
    struct flock fl;
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = sizeof(size_t);

    if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Could not aquire a lock on adcbuf pos lock.\n");
        return -1;
    } else {
        memcpy(pos, adcbuf->buf, sizeof(size_t));

        fl.l_type = F_UNLCK;
        if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
            fprintf(stderr, "Could not unlock adcbuf pos lock.\n");
            return -1;
        }
    }

    return 0;
}

int lpadc_set_pos(lpadcbuf_t * adcbuf, size_t pos) {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = sizeof(size_t);

    if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Could not aquire a read lock on adcbuf pos.\n");
        return -1;
    } else {
        memcpy(adcbuf->buf, &pos, sizeof(size_t));

        fl.l_type = F_UNLCK;
        if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
            fprintf(stderr, "Could not unlock read lock adcbuf pos.\n");
            return -1;
        }
    }

    return 0;
}

int lpadc_get_length(lpadcbuf_t * adcbuf, size_t * length) {
    struct flock fl;
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = sizeof(size_t);
    fl.l_len = sizeof(size_t);

    if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Could not aquire a lock on adcbuf length lock.\n");
        return -1;
    } else {
        memcpy(length, adcbuf->buf + sizeof(size_t), sizeof(size_t));

        fl.l_type = F_UNLCK;
        if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
            fprintf(stderr, "Could not unlock adcbuf length lock.\n");
            return -1;
        }
    }

    return 0;
}

int lpadc_set_length(lpadcbuf_t * adcbuf, size_t length) {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = sizeof(size_t);
    fl.l_len = sizeof(size_t);

    if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Could not aquire a read lock on adcbuf length.\n");
        return -1;
    } else {
        memcpy(adcbuf->buf + sizeof(size_t), &length, sizeof(size_t));

        fl.l_type = F_UNLCK;
        if(fcntl(adcbuf->fd, F_SETLK, &fl) == -1) {
            fprintf(stderr, "Could not unlock read lock adcbuf length.\n");
            return -1;
        }
    }

    return 0;
}

void lpadc_get_channels(lpadcbuf_t * adcbuf, int * channels) {
    memcpy(channels, adcbuf->buf + (sizeof(size_t)*2), sizeof(int));
}

void lpadc_set_channels(lpadcbuf_t * adcbuf, int channels) {
    memcpy(adcbuf->buf + (sizeof(size_t)*2), &channels, sizeof(int));
}

lpadcbuf_t * lpadc_create() {
    lpadcbuf_t * adcbuf;
    size_t bufoffset = 0;

    adcbuf = (lpadcbuf_t *)calloc(sizeof(lpadcbuf_t), 1);
    adcbuf->fd = shm_open(LPADC_BUFNAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if(adcbuf->fd == -1) {
        fprintf(stderr, "Could not get a handle to adcbuf shared memory.\n");
        exit(1);
    }

    /* FIXME get samplerate & channels from context */
    adcbuf->length = (size_t)(ASTRID_ADCSECONDS * ASTRID_SAMPLERATE);
    adcbuf->channels = ASTRID_CHANNELS;
    adcbuf->headsize = sizeof(size_t) + sizeof(size_t) + sizeof(int);
    adcbuf->fullsize = adcbuf->headsize + (sizeof(lpfloat_t) * adcbuf->length * adcbuf->channels);
    adcbuf->pos = 0;

    if(ftruncate(adcbuf->fd, adcbuf->fullsize) == -1) {
        fprintf(stderr, "Could not resize adcbuf.\n");
        return NULL;
    }

    adcbuf->buf = mmap(NULL, adcbuf->fullsize, PROT_READ | PROT_WRITE, MAP_SHARED, adcbuf->fd, 0);
    if(adcbuf->buf == MAP_FAILED) {
        fprintf(stderr, "Could not mmap adcbuf.\n");
        return NULL;
    }

    memcpy(adcbuf->buf, &adcbuf->pos, sizeof(size_t));
    bufoffset += sizeof(size_t);

    memcpy(adcbuf->buf+bufoffset, &adcbuf->length, sizeof(size_t));
    bufoffset += sizeof(size_t);

    memcpy(adcbuf->buf+bufoffset, &adcbuf->channels, sizeof(int));

    return adcbuf;
}

lpadcbuf_t * lpadc_open() {
    lpadcbuf_t * adcbuf;
    struct stat buffdstat;

    adcbuf = (lpadcbuf_t *)calloc(sizeof(lpadcbuf_t), 1);
    adcbuf->fd = shm_open(LPADC_BUFNAME, O_RDONLY, 0);
    if(adcbuf->fd == -1) {
        fprintf(stderr, "lpadc_open: Could not open a handle to adcbuf shared memory.\n");
        return NULL;
    }

    /* fstat check size */
    if(fstat(adcbuf->fd, &buffdstat) == -1) {
        fprintf(stderr, "lpadc_open: Could not fstat adcbuf shared memory.\n");
        return NULL;
    }

    adcbuf->fullsize = buffdstat.st_size;

    adcbuf->buf = mmap(NULL, adcbuf->fullsize, PROT_READ, MAP_SHARED, adcbuf->fd, 0);
    if(adcbuf->buf == MAP_FAILED) {
        fprintf(stderr, "lpadc_open: Could not mmap adcbuf.\n");
        return NULL;
    }

    adcbuf->headsize = sizeof(size_t) + sizeof(size_t) + sizeof(int);
    lpadc_get_pos(adcbuf, &adcbuf->pos);
    lpadc_get_channels(adcbuf, &adcbuf->channels);
    lpadc_get_length(adcbuf, &adcbuf->length);

    return adcbuf;
}

lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, size_t frame, int channel) {
    lpfloat_t sample;
    size_t offset;
    size_t length = 0;
    int channels = 0;

    // get buf channels
    lpadc_get_channels(adcbuf, &channels);
    lpadc_get_length(adcbuf, &length);

    frame = frame % length;

    // get offset to buf
    offset = adcbuf->headsize;
    offset += ((frame * channels + channel) * sizeof(lpfloat_t));

    // memcpy at offset to lpfloat_t
    sample = 0;
    memcpy(&sample, adcbuf->buf+offset, sizeof(lpfloat_t));

    return sample;
}

int lpadc_increment_pos(lpadcbuf_t * adcbuf, int count) {
    size_t pos = 0;
    size_t length = 0;

    if(lpadc_get_pos(adcbuf, &pos) == -1) return -1;
    if(lpadc_get_length(adcbuf, &length) == -1) return -1;
    pos = (pos + count) % length;

    return lpadc_set_pos(adcbuf, pos);
}

size_t lpadc_write_sample(lpadcbuf_t * adcbuf, lpfloat_t sample, size_t frame, int channel, ssize_t offset) {
    size_t writepos;
    size_t * length;
    int * channels;

    length = (size_t *)calloc(1, sizeof(size_t));
    channels = (int *)calloc(1, sizeof(int));

    lpadc_get_length(adcbuf, length);

    assert(*length != 0);

    frame = (frame + offset) % (*length);
    lpadc_get_channels(adcbuf, channels);
    writepos = adcbuf->headsize + (((frame * (*channels)) + channel) * sizeof(lpfloat_t));
    memcpy(adcbuf->buf + writepos, &sample, sizeof(lpfloat_t));

    free(length);
    free(channels);

    return frame;
}


int lpadc_close(lpadcbuf_t * adcbuf) {
    if(close(adcbuf->fd) == -1) {
        fprintf(stderr, "Could not close adcbuf fd.\n");
        return -1;
    }
    return 0;
}

int lpadc_destroy(lpadcbuf_t * adcbuf) {
    if(shm_unlink(LPADC_BUFNAME) == -1) {
        fprintf(stderr, "Could not unlink adcbuf fd.\n");
        return -1;
    }
    free(adcbuf);
    return 0;
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


