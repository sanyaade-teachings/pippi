#include "astrid.h"


int lpadc_create() {
    int shmid, count;
    FILE * handle;
    lpadcbuf_t adc;
    lpadcbuf_t * adcbuf;
    struct shmid_ds shm_info;

    /* Create or open the system V shared memory segment with 
     * permissions allowing world read/write access. (Easy interop?)
     *
     * The special IPC_PRIVATE key produces a unique identifier on each 
     * call to shmget, so all other operations on the shared memory segment 
     * are done through the identifier produced here. */
    shmid = shmget(IPC_PRIVATE, sizeof(lpadcbuf_t), IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmid < 0) {
        perror("shmget failed");
        return -1;
    }

    if(shmctl(shmid, IPC_STAT, &shm_info) == -1) {
        perror("shmctl failed");
        return -1;
    }

    /* Write the identifier for the adcbuf shared memory into a 
     * well known file other processes can use to attach and read */
    handle = fopen(LPADC_HANDLE, "w");
    if(handle == NULL) {
        perror("Could not open tmpfile to write adcbuf shmid");
        return -1;
    }

    count = fprintf(handle, "%d", shmid);
    if(count < 0) {
        perror("Could not write shmid to tmpfile");
        fclose(handle);
        return -1;
    }

    adcbuf = (lpadcbuf_t *)shmat(shmid, NULL, 0);
    if(adcbuf == (void *)-1) {
        perror("Could not attach to shared memory");
        fclose(handle);
        return -1;
    }

    adc.pos = 0;
    memset(adc.buf, 0, sizeof(adc.buf));
    memcpy(adcbuf, &adc, sizeof(lpadcbuf_t));

    fclose(handle);
    return 0;
}

int lpadc_getid() {
    int handle, shmid = 0;
    char * shmidp;
    struct stat st;

    /* Read the identifier for the adcbuf shared memory 
     * from the well known file. Reading with mmap to speed 
     * up access as this might be called in a tight render loop */
    handle = open(LPADC_HANDLE, O_RDONLY);
    if(handle < 0) {
        perror("Could not open tmpfile to read adcbuf shmid");
        return -1;
    }

    /* Get the file size */
    if(fstat(handle, &st) == -1) {
        perror("Could not stat tmpfile");
        close(handle);
        return -1;
    }

    /* Map the file into memory */
    shmidp = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, handle, 0);
    if(shmidp == MAP_FAILED) {
        perror("Could not mmap tmpfile");
        close(handle);
        return -1;
    }

    /* Copy the identifier from the mmaped file & cleanup */
    sscanf(shmidp, "%d", &shmid);
    munmap(shmidp, st.st_size);
    close(handle);

    return shmid;
}

lpadcbuf_t * lpadc_open() {
    int shmid;

    /* Get the shared memory id */
    shmid = lpadc_getid();

    /* Attach to the shared memory segment and 
     * populate the adcbuf pointer. */
    return (lpadcbuf_t *)shmat(shmid, NULL, 0);
}

void lpadc_write_block(lpadcbuf_t * adcbuf, float * block, size_t blocksize_in_bytes) {
    size_t byte_pos, i;

    byte_pos = atomic_load_explicit(&adcbuf->pos, memory_order_acquire);

    /* Copy the block */
    if(byte_pos + blocksize_in_bytes >= LPADCBUFSIZE) {
        for(i=0; i < blocksize_in_bytes; i++) {
            memcpy(adcbuf->buf + ((byte_pos+i) % LPADCBUFSIZE), block+i, 1);
        }
    } else {
        memcpy(adcbuf->buf + byte_pos, block, blocksize_in_bytes);
    }

    /* Increment the position */
    byte_pos += blocksize_in_bytes;
    byte_pos = byte_pos % LPADCBUFSIZE;

    atomic_store_explicit(&adcbuf->pos, byte_pos, memory_order_release);
}

lpfloat_t lpadc_read_sample(lpadcbuf_t * adcbuf, size_t offset) {
    size_t byte_offset;
    float sample=0;

    byte_offset = atomic_load_explicit(&adcbuf->pos, memory_order_acquire);
    byte_offset = byte_offset - offset;
    byte_offset = byte_offset % (LPADCBUFSIZE-sizeof(float));

    memcpy(&sample, (char*)(&adcbuf->buf) + byte_offset, sizeof(float));
    return (lpfloat_t)sample;
}

int lpadc_destroy() {
    int shmid;
    shmid = lpadc_getid();
    if(shmctl(shmid, IPC_RMID, 0) < 0) {
        perror("Could not destroy adcbuf shared memory");
        return -1;
    }
    return 0;
}

char * serialize_buffer(lpbuffer_t * buf, lpmsg_t * msg) {
    size_t strsize, audiosize, offset;
    char * str;

    audiosize = buf->length * buf->channels * sizeof(lpfloat_t);

    strsize =  0;
    strsize += sizeof(size_t);  /* audio len     */
    strsize += sizeof(int);     /* channels   */
    strsize += sizeof(int);     /* samplerate */
    strsize += sizeof(int);     /* is_looping */
    strsize += sizeof(size_t);  /* onset      */
    strsize += audiosize;       /* audio data */
    strsize += sizeof(lpmsg_t); /* message */

    /* initialize string buffer */
    str = calloc(1, strsize);

    offset = 0;

    memcpy(str + offset, &audiosize, sizeof(size_t));
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

    memcpy(str + offset, msg, sizeof(lpmsg_t));
    offset += sizeof(lpmsg_t);

    return str;
}

lpbuffer_t * deserialize_buffer(char * str, lpmsg_t * msg) {
    size_t audiosize, offset, length, onset;
    int channels, samplerate, is_looping;
    lpbuffer_t * buf;
    lpfloat_t * audio;

    offset = 0;

    memcpy(&audiosize, str + offset, sizeof(size_t));
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

    memcpy(msg, str + offset, sizeof(lpmsg_t));
    offset += sizeof(lpmsg_t);

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

int get_play_message(char * instrument_name, lpmsg_t * msg) {
    int qfd;
    ssize_t qname_length;
    char qname[LPMAXQNAME] = {0};
    ssize_t read_result;

    qname_length = snprintf(NULL, 0, "%s-%s", LPPLAYQ, instrument_name) + 1;
    qname_length = (LPMAXQNAME >= qname_length) ? LPMAXQNAME : qname_length;
    snprintf(qname, qname_length, "%s-%s", LPPLAYQ, instrument_name);

    umask(0);
    if(mkfifo(qname, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        perror("Error creating named pipe");
        return -1;
    }

    qfd = open(qname, O_RDONLY);
    if((read_result = read(qfd, msg, sizeof(lpmsg_t))) != sizeof(lpmsg_t)) {
        syslog(LOG_INFO, "Play queue for %s has closed\n", instrument_name);
        return -1;
    }

    if(close(qfd) == -1) {
        perror("Error closing q");
        return -1; 
    }

    return 0;
}

int astrid_playq_open(char * instrument_name) {
    int qfd;
    ssize_t qname_length;
    char qname[LPMAXQNAME] = {0};

    qname_length = snprintf(NULL, 0, "%s-%s", LPPLAYQ, instrument_name) + 1;
    qname_length = (LPMAXQNAME >= qname_length) ? LPMAXQNAME : qname_length;
    snprintf(qname, qname_length, "%s-%s", LPPLAYQ, instrument_name);

    umask(0);
    if(mkfifo(qname, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        perror("Error creating play queue FIFO");
        return -1;
    }

    if((qfd = open(qname, O_RDWR)) < 0) {
        perror("Error opening play queue FIFO");
    };

    return qfd;
}

int astrid_playq_close(int qfd) {
    if(close(qfd) == -1) {
        perror("Error closing q");
        return -1; 
    }

    return 0;
}

int astrid_playq_read(int qfd, lpmsg_t * msg) {
    ssize_t read_result;
    read_result = read(qfd, msg, sizeof(lpmsg_t));
    if(read_result == 0) {
        syslog(LOG_DEBUG, "The play queue (%d) has been closed. (EOF)\n", qfd);
        return -1;
    }

    // do we need larger play messages?
    if(read_result != sizeof(lpmsg_t)) {
        syslog(LOG_INFO, "The play queue (%d) returned %d bytes. Expecting sizeof(lpmsg_t)==%d\n", qfd, (int)read_result, (int)sizeof(lpmsg_t));
        return -1;
    }

    return 0;
}

int send_play_message(lpmsg_t * msg) {
    char qname[LPMAXQNAME] = {0};
    ssize_t qname_length;
    int qfd;

    qname_length = snprintf(NULL, 0, "%s-%s", LPPLAYQ, msg->instrument_name) + 1;
    qname_length = (LPMAXQNAME >= qname_length) ? LPMAXQNAME : qname_length;
    snprintf(qname, qname_length, "%s-%s", LPPLAYQ, msg->instrument_name);

    umask(0);
    if(mkfifo(qname, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        perror("Error creating named pipe");
        return -1;
    }

    qfd = open(qname, O_WRONLY);

    //msg->delay = 0;

    if(write(qfd, msg, sizeof(lpmsg_t)) != sizeof(lpmsg_t)) {
        perror("Could not write to q");
        return -1;
    }

    if(close(qfd) == -1) {
        perror("Error closing play q");
        return -1; 
    }

    return 0;
}

