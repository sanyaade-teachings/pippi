#include "astrid.h"

void lptimeit_since(struct timespec * start) {
    struct timespec now;
    long long elapsed;

    clock_gettime(CLOCK_REALTIME, &now);
    elapsed = (now.tv_sec - start->tv_sec) * 1e9 + (now.tv_nsec - start->tv_nsec);
    printf("%lld nanoseconds\n", elapsed);

    start->tv_sec = now.tv_sec;
    start->tv_nsec = now.tv_nsec;
}

int lpcounter_read_and_increment(lpcounter_t * c) {
    struct sembuf sop;
    size_t * counter;
    int counter_value;

    /* Aquire lock */
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if(semop(c->semid, &sop, 1) < 0) {
        perror("semop");
        return -1;
    }

    /* Read the current value */
    counter = shmat(c->shmid, NULL, 0);
    counter_value = *counter;

    /* Increment the counter */
    *counter += 1;

    /* Release lock */
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if(semop(c->semid, &sop, 1) < 0) {
        perror("semop");
        return -1;
    }

    return counter_value;
}

int lpcounter_destroy(lpcounter_t * c) {
    union semun dummy;

    /* Remove the semaphore and shared memory counter */
    if(shmctl(c->shmid, IPC_RMID, NULL) < 0) {
        perror("shmctl");
        return -1;
    }

    if(semctl(c->semid, 0, IPC_RMID, dummy) < 0) {
        perror("semctl");
        return -1;
    }

    /* Don't need to free the struct since it's just two 
     * ints on the stack... */
    return 0;
}

int lpcounter_create(lpcounter_t * c) {
    size_t * counter;
    union semun arg;

    /* Create the shared memory space for the counter value */
    c->shmid = shmget(IPC_PRIVATE, sizeof(size_t), IPC_CREAT | 0600);
    if (c->shmid < 0) {
        perror("shmget");
        return -1;
    }

    /* Create the semaphore used as a read/write lock on the counter */
    c->semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (c->semid < 0) {
        perror("semget");
        return -1;
    }

    /* Attach the shared memory for reading and writing */
    counter = shmat(c->shmid, NULL, 0);
    if (counter == (void*)-1) {
        perror("shmat");
        return -1;
    }

    /* Start the voice IDs at 1 */
    *counter = 1;

    /* Prime the lock by initalizing the sempahore to 1 */
    arg.val = 1;
    if(semctl(c->semid, 0, SETVAL, arg) < 0) {
        perror("semctl");
        return -1;
    }

    return 0;
}

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

int lpipc_setid(char * path, int id) {
    FILE * handle;
    int count;

    handle = fopen(path, "w");
    if(handle == NULL) {
        perror("fopen");
        return -1;
    }

    count = fprintf(handle, "%d", id);
    if(count < 0) {
        perror("fprintf");
        fclose(handle);
        return -1;
    }

    return 0;
}

int lpipc_getid(char * path) {
    int handle, id = 0;
    char * idp;
    struct stat st;

    /* Read the identifier from the well known file. mmap speeds 
     * up access when ids need to be fetched when timing matters */
    handle = open(path, O_RDONLY);
    if(handle < 0) {
        perror("lpipc_getid open");
        return -1;
    }

    /* Get the file size */
    if(fstat(handle, &st) == -1) {
        perror("lpipc_getid fstat");
        close(handle);
        return -1;
    }

    /* Map the file into memory */
    idp = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, handle, 0);
    if(idp == MAP_FAILED) {
        perror("lpipc_getid mmap");
        close(handle);
        return -1;
    }

    /* Copy the identifier from the mmaped file & cleanup */
    sscanf(idp, "%d", &id);
    munmap(idp, st.st_size);
    close(handle);

    return id;
}

int lpadc_getid() {
    return lpipc_getid(LPADC_HANDLE);
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

    syslog(LOG_DEBUG, "astrid_playq_open: %s\n", instrument_name);

    qname_length = snprintf(NULL, 0, "%s-%s", LPPLAYQ, instrument_name) + 1;
    qname_length = (LPMAXQNAME >= qname_length) ? LPMAXQNAME : qname_length;
    snprintf(qname, qname_length, "%s-%s", LPPLAYQ, instrument_name);

    syslog(LOG_ERR, "astrid_playq_open: %s\n", qname);

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

    syslog(LOG_DEBUG, " Q           %s MSG READ FROM Q %d\n", msg->instrument_name, qfd);
    syslog(LOG_DEBUG, " Q           %s (msg.instrument_name)\n", msg->instrument_name);
    syslog(LOG_DEBUG, " Q           %d (msg.voice_id)\n", (int)msg->voice_id);
    syslog(LOG_DEBUG, " Q           %d (msg.delay)\n", (int)msg->delay);

    return 0;
}

int send_play_message(lpmsg_t msg) {
    char qname[LPMAXQNAME] = {0};
    ssize_t qname_length;
    int qfd;

    qname_length = snprintf(NULL, 0, "%s-%s", LPPLAYQ, msg.instrument_name) + 1;
    qname_length = (LPMAXQNAME >= qname_length) ? LPMAXQNAME : qname_length;
    snprintf(qname, qname_length, "%s-%s", LPPLAYQ, msg.instrument_name);

    umask(0);
    if(mkfifo(qname, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        perror("Error creating named pipe");
        return -1;
    }

    qfd = open(qname, O_WRONLY);

    if(write(qfd, &msg, sizeof(lpmsg_t)) != sizeof(lpmsg_t)) {
        perror("Could not write to q");
        return -1;
    }

    if(close(qfd) == -1) {
        perror("Error closing play q");
        return -1; 
    }

    syslog(LOG_DEBUG, " P           %s MSG OUT %s\n", msg.instrument_name, msg.msg);
    syslog(LOG_DEBUG, " P           %s (msg.instrument_name)\n", msg.instrument_name);
    syslog(LOG_DEBUG, " P           %d (msg.voice_id)\n", (int)msg.voice_id);
    syslog(LOG_DEBUG, " P           %d (msg.delay)\n", (int)msg.delay);


    return 0;
}

void scheduler_get_now(struct timespec * now) {
    clock_gettime(CLOCK_MONOTONIC_RAW, now);
}

void scheduler_frames_to_timespec(
    size_t frames, 
    lpfloat_t samplerate, 
    struct timespec * ts
) {
    size_t ns;

    ts->tv_sec = (time_t)(frames / samplerate);

    ns = frames - (size_t)(ts->tv_sec * samplerate);
    ts->tv_nsec = (size_t)(ns / samplerate);
}

void scheduler_increment_timespec_by_ns(
    struct timespec * ts, 
    size_t ns
) {
    size_t seconds = 0;
    while(ns >= 1000000000) {
        ns -= 1000000000;
        seconds += 1;
    }

    ts->tv_sec += seconds;
    ts->tv_nsec += ns;
}

size_t scheduler_timespec_to_ticks(
    lpscheduler_t * s, 
    struct timespec * ts
) {
    size_t secs, nsecs, total_ns;     

    /* Seconds and nanoseconds since init */
    secs = ts->tv_sec - s->init->tv_sec;
    nsecs = ts->tv_nsec - s->init->tv_nsec;

    /* Convert nanoseconds since init to number of 
     * ticks since init.
     *
     * ticks advance on frame boundries as the scheduler 
     * processes each frame of output.
     *
     * `tick_ns` is the number of nanoseconds 
     * per frame at the current samplerate.
     **/
    total_ns = nsecs + (secs * 1000000000);
    return total_ns / s->tick_ns;
}

int scheduler_onset_has_elapsed(lpscheduler_t * s, struct timespec * ts) {
    printf(" s->now->tv_sec: %d\n", (int)s->now->tv_sec);
    printf("     ts->tv_sec: %d\n", (int)ts->tv_sec);
    printf("s->now->tv_nsec: %d\n", (int)s->now->tv_nsec);
    printf("    ts->tv_nsec: %d\n", (int)ts->tv_nsec);

    return ((
            s->now->tv_sec >= ts->tv_sec
        ) || (
            s->now->tv_sec == ts->tv_sec &&
            s->now->tv_nsec >= ts->tv_nsec
        )
    ) ? 1 : 0;
}

void ll_display(lpevent_t * head) {
    lpevent_t * current;
    if(head) {
        current = head;
        while(current->next != NULL) {
            printf("    e%d onset: %d pos: %d length: %d\n", (int)current->id, (int)current->onset, (int)current->pos, (int)current->buf->length);
            current = (lpevent_t *)current->next;        
        }
        printf("    e%d onset: %d pos: %d length: %d\n", (int)current->id, (int)current->onset, (int)current->pos, (int)current->buf->length);
    }
}

int ll_count(lpevent_t * head) {
    lpevent_t * current;
    int count;

    count = 0;

    if(head) {
        count = 1;
        current = head;
        while(current->next != NULL) {
            current = (lpevent_t *)current->next;        
            count += 1;
        }
    }

    return count;
}

/* Add event to the tail of the waiting queue */
static inline void start_waiting(lpscheduler_t * s, lpevent_t * e) {
    lpevent_t * current;

    if(s->waiting_queue_head == NULL) {
        s->waiting_queue_head = e;
    } else {
        current = s->waiting_queue_head;
        while(current->next != NULL) {
            current = (lpevent_t *)current->next;        
        }
        current->next = (void *)e;
    }
} 

static inline void start_playing(lpscheduler_t * s, lpevent_t * e) {
    lpevent_t * current;
    lpevent_t * prev;

    /* Remove from the waiting queue */
    if(s->waiting_queue_head == NULL) {
        printf("Cannot move this event. There is nothing in the waiting queue!\n");
    }

    prev = NULL;
    current = s->waiting_queue_head;
    while(current != e && current->next != NULL) {
        prev = current;
        current = (lpevent_t *)current->next;
    }
    if(prev) {
        prev->next = current->next;
    } else {
        s->waiting_queue_head = (lpevent_t *)current->next;
    }

    current->next = NULL;

    /* Add to the tail of the playing queue */
    if(s->playing_stack_head == NULL) {
        s->playing_stack_head = e;
    } else {
        current = s->playing_stack_head;
        while(current->next != NULL) {
            current = (lpevent_t *)current->next;
        }
        current->next = (void *)e;
    }
}

static inline void stop_playing(lpscheduler_t * s, lpevent_t * e) {
    lpevent_t * current;
    lpevent_t * prev;

    /* Remove from the playing stack */
    if(s->playing_stack_head == NULL) {
        printf("Cannot move this event. There is nothing in the waiting queue!\n");
    }

    prev = NULL;
    current = s->playing_stack_head;
    while(current != e && current->next != NULL) {
        prev = current;
        current = (lpevent_t *)current->next;
    }

    if(prev) {
        prev->next = current->next;
    } else {
        s->playing_stack_head = (lpevent_t *)current->next;
    }

    current->next = NULL;

    /* Add to the tail of the garbage stack */
    if(s->nursery_head == NULL) {
        s->nursery_head = e;
    } else {
        current = s->nursery_head;
        while(current->next != NULL) {
            current = (lpevent_t *)current->next;
        }
        current->next = (void *)e;
    }
}

lpscheduler_t * scheduler_create(int realtime, int channels, lpfloat_t samplerate) {
    lpscheduler_t * s;

    s = (lpscheduler_t *)LPMemoryPool.alloc(1, sizeof(lpscheduler_t));
    s->now = (struct timespec *)LPMemoryPool.alloc(1, sizeof(struct timespec));

    s->realtime = realtime;

    s->waiting_queue_head = NULL;
    s->playing_stack_head = NULL;
    s->nursery_head = NULL;

    s->samplerate = samplerate;
    s->channels = channels;

    s->tick_ns = (size_t)(samplerate / 1000000000.f);

    if(realtime == 1) scheduler_get_now(s->now);
    s->ticks = 0;
    s->current_frame = (lpfloat_t *)LPMemoryPool.alloc(channels, sizeof(lpfloat_t));

    s->event_count = 0;

    return s;
}

/* look for events waiting to be scheduled */
static inline void scheduler_update(lpscheduler_t * s) {
    lpevent_t * current;
    void * next;
    if(s->waiting_queue_head != NULL) {
        current = s->waiting_queue_head;
        while(current->next != NULL) {
            next = current->next;
            if(s->ticks >= current->onset) {
                start_playing(s, current);
            }
            current = (lpevent_t *)next;
        }
        if(s->ticks >= current->onset) {
            start_playing(s, current);
        }
    }

    /* look for events that have finished playing */
    if(s->playing_stack_head != NULL) {
        current = s->playing_stack_head;
        while(current->next != NULL) {
            next = current->next;
            if(current->pos >= current->buf->length-1) {
                stop_playing(s, current);
            }
            current = (lpevent_t *)next;
        }
        if(current->pos >= current->buf->length-1) {
            stop_playing(s, current);
        }


    }
}


static inline void scheduler_mix_buffers(lpscheduler_t * s) {
    lpevent_t * current;
    lpfloat_t sample;
    int bufc, c;

    if(s->playing_stack_head == NULL) {
        for(c=0; c < s->channels; c++) {
            s->current_frame[c] = 0.f;
        }
        return;
    }

    for(c=0; c < s->channels; c++) {
        sample = 0.f;

        current = s->playing_stack_head;
        while(current->next != NULL) {
            if(current->pos < current->buf->length) {
                bufc = c % current->buf->channels;
                sample += current->buf->data[current->pos * current->buf->channels + bufc];
            }
            current = (lpevent_t *)current->next;
        }

        if(current->pos < current->buf->length) {
            bufc = c % current->buf->channels;
            sample += current->buf->data[current->pos * current->buf->channels + bufc];
        }
 
        s->current_frame[c] = sample;
    }
}

static inline void scheduler_try_callback(lpscheduler_t * s, lpevent_t * e) {
    /* Execute callback if one has been registered */
    if(
        e->callback_fired == 0 && 
        e->callback != NULL && 
        s->ticks >= e->callback_onset
    ) {
        e->callback(e->msg);
        e->callback_fired = 1;
    }
}

static inline void scheduler_advance_buffers(lpscheduler_t * s) {
    lpevent_t * current;
 
    /* loop over buffers and advance their positions */
    if(s->playing_stack_head != NULL) {
        current = s->playing_stack_head;
        while(current->next != NULL) {
            current->pos += 1;
            current = (lpevent_t *)current->next;
        }
        current->pos += 1;
    }
}

void scheduler_debug(lpscheduler_t * s) {
    if(s->waiting_queue_head) {
        printf("%d waiting\n", ll_count(s->waiting_queue_head));
        ll_display(s->waiting_queue_head);
    } else {
        printf("none waiting\n");
    }

    if(s->playing_stack_head) {
        printf("%d playing\n", ll_count(s->playing_stack_head));
        ll_display(s->playing_stack_head);
    } else {
        printf("none playing\n");
    }

    if(s->nursery_head) {
        printf("%d done\n\n", ll_count(s->nursery_head));
        ll_display(s->nursery_head);
    } else {
        printf("none done\n");
    }
}

void lpscheduler_handle_callbacks(lpscheduler_t * s) {
    lpevent_t * current;
    if(s->playing_stack_head != NULL) {
        current = s->playing_stack_head;
        scheduler_try_callback(s, current);
        while(current->next != NULL) {
            current = (lpevent_t *)current->next;
            scheduler_try_callback(s, current);
        }
    }
}

void lpscheduler_tick(lpscheduler_t * s) {
    //scheduler_debug(s);

    /* Move buffers to proper lists */
    scheduler_update(s);

    /* Mix currently playing buffers into s->current_frame */
    scheduler_mix_buffers(s);

    /* Advance the position for all playing buffers */
    scheduler_advance_buffers(s);

    /* Increment process ticks and update now timestamp */
    s->ticks += 1;
    if(s->realtime == 1) {
        scheduler_get_now(s->now);
    } else {
        scheduler_increment_timespec_by_ns(s->now, s->tick_ns);
    }
}

void scheduler_schedule_event(lpscheduler_t * s, 
        lpbuffer_t * buf, 
        size_t delay, 
        void (*callback)(lpmsg_t), 
        lpmsg_t msg,
        size_t callback_delay
) {
    lpevent_t * e;

    if(s->nursery_head != NULL) {
        e = s->nursery_head;
        s->nursery_head = (void *)e->next;
        e->next = NULL; 
    } else {
        e = (lpevent_t *)LPMemoryPool.alloc(1, sizeof(lpevent_t));
        s->event_count += 1;
        e->id = s->event_count;
        e->onset = 0;
        e->callback_onset = 0;
    }

    e->buf = buf;
    e->pos = 0;
    e->onset = s->ticks + delay;
    e->callback = callback;
    e->msg = msg;

    e->callback_onset = e->onset + callback_delay;
    e->callback_fired = 0;

    start_waiting(s, e);
}

int scheduler_count_waiting(lpscheduler_t * s) {
    return ll_count(s->waiting_queue_head);
}

int scheduler_count_playing(lpscheduler_t * s) {
    return ll_count(s->playing_stack_head);
}

int scheduler_count_done(lpscheduler_t * s) {
    return ll_count(s->nursery_head);
}

int scheduler_is_playing(lpscheduler_t * s) {
    int playing;
    playing = 0;
    if(scheduler_count_waiting(s) > 0) playing = 1;
    if(scheduler_count_playing(s) > 0) playing = 1;
    return playing;
}

void scheduler_destroy(lpscheduler_t * s) {
    /* Loop over queues and free buffers, events */
    lpevent_t * current;
    lpevent_t * next;

    if(s->waiting_queue_head) {
        current = s->waiting_queue_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            LPMemoryPool.free(current);
            current = (lpevent_t *)next;        
        }
        LPMemoryPool.free(current);
    }

    if(s->playing_stack_head) {
        current = s->playing_stack_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            LPMemoryPool.free(current);
            current = (lpevent_t *)next;        
        }
        LPMemoryPool.free(current);
    }

    if(s->nursery_head) {
        current = s->nursery_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            LPMemoryPool.free(current);
            current = (lpevent_t *)next;        
        }
        LPMemoryPool.free(current);
    }
    LPMemoryPool.free(s->current_frame);
    LPMemoryPool.free(s);
}

void scheduler_cleanup_nursery(lpscheduler_t * s) {
    /* Loop over nursey and free buffers */
    lpevent_t * current;
    lpevent_t * next;

    if(s->nursery_head != NULL) {
        current = s->nursery_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            current->next = NULL;
            LPBuffer.destroy(current->buf);
            current = (lpevent_t *)next;        
        }
    }
}

