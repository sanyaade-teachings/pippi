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

/* sqlite3 is pretty slow to build, so sessiondb are 
 * disabled for most astrid modules */
#ifdef LPSESSIONDB
/* SESSION
 * DATABASE
 * ********/
static int lpsessiondb_callback_debug(void * unused, int argc, char ** argv, char ** colname) {
    int i;
    for(i=0; i < argc; i++) {
        syslog(LOG_DEBUG, "lpsessiondb_callback %s=%s\n", colname[i], argv[i] ? argv[i] : "None");
    }
    return 0;
}

static int lpsessiondb_callback_noop(void * unused, int argc, char ** argv, char ** colname) {
    return 0;
}

int lpsessiondb_open_for_writing(sqlite3 ** db) {
    if(sqlite3_open_v2(ASTRID_SESSIONDB_PATH, db, SQLITE_OPEN_READWRITE, NULL) > 0) {
        syslog(LOG_ERR, "Could not open db at path: %s. Error: %s\n", ASTRID_SESSIONDB_PATH, strerror(errno));
        return -1;
    }
    return 0;
}

int lpsessiondb_open_for_reading(sqlite3 ** db) {
    if(sqlite3_open_v2(ASTRID_SESSIONDB_PATH, db, SQLITE_OPEN_READONLY, NULL) > 0) {
        syslog(LOG_ERR, "Could not open db at path: %s. Error: %s\n", ASTRID_SESSIONDB_PATH, strerror(errno));
        return -1;
    }
    return 0;
}


int lpsessiondb_close(sqlite3 * db) {
    if(sqlite3_close(db) < 0) {
        syslog(LOG_ERR, "Could not close sql db. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


int lpsessiondb_create(sqlite3 ** db) {
    char * err = 0;
    char * sql = "create table voices \
                  (created integer, started integer, last_render integer, ended integer, \
                   active integer, timestamp real, id integer, instrument_name text, \
                   params text, render_count integer);";

    /* Remove any existing sessiondb */
    unlink(ASTRID_SESSIONDB_PATH);

    /* Create and open the database */
    if(sqlite3_open_v2(ASTRID_SESSIONDB_PATH, db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) > 0) {
        syslog(LOG_ERR, "Could not open db at path: %s. Error: %s\n", ASTRID_SESSIONDB_PATH, strerror(errno));
        return -1;
    }

    /* Set up session schema */
    if(sqlite3_exec(*db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "Could not exec sql statement: %s. Error: %s\n", sql, sqlite3_errmsg(*db));
        return -1;
    }

    /* Set WAL mode */
    if(sqlite3_exec(*db, "pragma journal_mode=WAL;", lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "Could not set sessiondb WAL mode. Error: %s\n", sqlite3_errmsg(*db));
        return -1;
    }

    return 0;
}

int lpsessiondb_insert_voice(lpmsg_t msg) {
    sqlite3 * db;
    char * err = 0;
    char * sql;
    size_t sqlsize;
    struct timespec ts;
    long long now;

    char * _sql = "insert into voices (created, started, last_render, ended, active, timestamp, \
                   id, instrument_name, params, render_count) \
                   values (%lld, NULL, NULL, NULL, 0, %f, %d, \"%s\", \"%s\", 0);";

    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    /* Prepare the sql unsafely */
    sqlsize = snprintf(NULL, 0, _sql, now, msg.timestamp, (int)msg.voice_id, msg.instrument_name, msg.msg);
    sql = calloc(1, sqlsize+1);
    if(snprintf(sql, sqlsize, _sql, now, msg.timestamp, (int)msg.voice_id, msg.instrument_name, msg.msg) < 0) {
        syslog(LOG_ERR, "Could not concat sql for insert. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Open the sessiondb for writing */
    lpsessiondb_open_for_writing(&db);

    /* Insert the voice */
    if(sqlite3_exec(db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "Could not exec sql statement: %s. Error: %s\n", sql, strerror(errno));
        return -1;
    }

    return lpsessiondb_close(db);
}

int lpsessiondb_mark_voice_active(sqlite3 * db, int voice_id) {
    char * err = 0;
    char * sql;
    size_t sqlsize;
    struct timespec ts;
    long long now;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    /* Prepare the sql unsafely */
    sqlsize = snprintf(NULL, 0, "update voices set active=1, started=%lld, last_render=%lld, render_count=1 where id=%d;", now, now, voice_id);
    sql = calloc(1, sqlsize+1);
    if(snprintf(sql, sqlsize, "update voices set active=1, started=%lld, last_render=%lld, render_count=1 where id=%d;", now, now, voice_id) < 0) {
        syslog(LOG_ERR, "Could not concat sql for update. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Mark the voice as active */
    if(sqlite3_exec(db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "Could not exec sql statement: %s. Error: %s\n", sql, strerror(errno));
        return -1;
    }

    return 0;
}

int lpsessiondb_increment_voice_render_count(sqlite3 * db, int voice_id, size_t count) {
    char * err = 0;
    char * sql;
    size_t sqlsize;
    struct timespec ts;
    long long now;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    /* Prepare the sql unsafely */
    sqlsize = snprintf(NULL, 0, "update voices set active=1, last_render=%lld, render_count=%ld where id=%d;", now, count, voice_id);
    sql = calloc(1, sqlsize+1);
    if(snprintf(sql, sqlsize, "update voices set active=1, last_render=%lld, render_count=%ld where id=%d;", now, count, voice_id) < 0) {
        syslog(LOG_ERR, "Could not concat sql for update. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Mark the voice as active */
    if(sqlite3_exec(db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "Could not exec sql statement: %s. Error: %s\n", sql, strerror(errno));
        return -1;
    }

    return 0;
}

int lpsessiondb_mark_voice_stopped(sqlite3 * db, int voice_id, size_t count) {
    char * err = 0;
    char * sql;
    size_t sqlsize;
    struct timespec ts;
    long long now;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    /* Prepare the sql unsafely */
    sqlsize = snprintf(NULL, 0, "update voices set active=0, ended=%lld, last_render=%lld, render_count=%ld where id=%d;", now, now, count, voice_id);
    sql = calloc(1, sqlsize+1);
    if(snprintf(sql, sqlsize, "update voices set active=0, ended=%lld, last_render=%lld, render_count=%ld where id=%d;", now, now, count, voice_id) < 0) {
        syslog(LOG_ERR, "Could not concat sql for update. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Mark the voice as stopped */
    if(sqlite3_exec(db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "Could not exec sql statement: %s. Error: %s\n", sql, strerror(errno));
        return -1;
    }

    return 0;
}


#endif

/* THREAD SAFE
 * COUNTER TOOLS
 * *************/
int lpcounter_read_and_increment(lpcounter_t * c) {
    struct sembuf sop;
    size_t * counter;
    int counter_value;

    /* Aquire lock */
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if(semop(c->semid, &sop, 1) < 0) {
        syslog(LOG_ERR, "lpcounter_read_and_increment semop. Error: %s\n", strerror(errno));
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
        syslog(LOG_ERR, "lpcounter_read_and_increment semop. Error: %s\n", strerror(errno));
        return -1;
    }

    return counter_value;
}

int lpcounter_destroy(lpcounter_t * c) {
    union semun dummy;

    /* Remove the semaphore and shared memory counter */
    if(shmctl(c->shmid, IPC_RMID, NULL) < 0) {
        syslog(LOG_ERR, "lpcounter_destroy shmctl. Error: %s\n", strerror(errno));
        return -1;
    }

    if(semctl(c->semid, 0, IPC_RMID, dummy) < 0) {
        syslog(LOG_ERR, "lpcounter_destroy semctl. Error: %s\n", strerror(errno));
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
        syslog(LOG_ERR, "lpcounter_create shmget. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Create the semaphore used as a read/write lock on the counter */
    c->semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (c->semid < 0) {
        syslog(LOG_ERR, "lpcounter_create semget. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Attach the shared memory for reading and writing */
    counter = shmat(c->shmid, NULL, 0);
    if (counter == (void*)-1) {
        syslog(LOG_ERR, "lpcounter_create shmat. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Start the voice IDs at 1 */
    *counter = 1;

    /* Prime the lock by initalizing the sempahore to 1 */
    arg.val = 1;
    if(semctl(c->semid, 0, SETVAL, arg) < 0) {
        syslog(LOG_ERR, "lpcounter_create semctl. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/* SHARED MEMORY
 * COMMUNICATION TOOLS
 * *******************/
int lpipc_setid(char * path, int id) {
    FILE * handle;
    int count;

    handle = fopen(path, "w");
    if(handle == NULL) {
        syslog(LOG_ERR, "lpipc_setid fopen: %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    count = fprintf(handle, "%d", id);
    if(count < 0) {
        syslog(LOG_ERR, "lpipc_setid fprintf: %s. Error: %s\n", path, strerror(errno));
        fclose(handle);
        return -1;
    }

    fclose(handle);
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
        syslog(LOG_ERR, "lpipc_getid could not open path: %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    /* Get the file size */
    if(fstat(handle, &st) == -1) {
        syslog(LOG_ERR, "lpipc_getid fstat: %s. Error: %s\n", path, strerror(errno));
        close(handle);
        return -1;
    }

    /* Map the file into memory */
    idp = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, handle, 0);
    if(idp == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_getid mmap: %s. Error: %s\n", path, strerror(errno));
        close(handle);
        return -1;
    }

    /* Copy the identifier from the mmaped file & cleanup */
    sscanf(idp, "%d", &id);
    munmap(idp, st.st_size);
    close(handle);

    return id;
}

int lpipc_setvalue(char * path, void * value, size_t size) {
    int fd, bytes_written;

    if((fd = open(path, O_RDWR | O_CREAT, 0777)) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue open: %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    if((bytes_written = write(fd, value, size)) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue write: %s. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int lpipc_getvalue(char * path, void ** value, size_t size) {
    int fd;
    void * valuep;

    /* Get the file descriptor for the path */
    if((fd = open(path, O_RDONLY)) < 0) {
        syslog(LOG_ERR, "lpipc_getvalue could not open path: %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    /* Map the contents of the file into memory */
    if((valuep = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_getvalue mmap: %s. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    /* Copy the address of the mmaped file & cleanup */
    *value = valuep;
    close(fd);

    return 0;
}


/* SHARED MEMORY
 * BUFFER TOOLS
 * ************/
int lpipc_buffer_create(char * id_path, size_t length, int channels, int samplerate) {
    int shmid, semid;
    lpipc_buffer_t * buf;
    size_t bufsize;
    union semun arg;
    char * sempath;
    size_t sempath_size;


    /* Determine the size of the shared memory segment */
    bufsize = sizeof(lpipc_buffer_t) + (sizeof(lpfloat_t) * length * channels);

    /* Create the system V shared memory segment */
    shmid = shmget(IPC_PRIVATE, bufsize, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmid < 0) {
        syslog(LOG_ERR, "lpadc_create shmget. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Write the identifier for the shared memory buffer into a 
     * well known file other processes can use to attach and read */
    if(lpipc_setid(id_path, shmid) < 0) {
        syslog(LOG_ERR, "lpadc_create failed to store shared memory ID to path %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }


    /* Attach the shared memory */
    buf = (lpipc_buffer_t *)shmat(shmid, NULL, 0);
    if(buf == (void *)-1) {
        syslog(LOG_ERR, "lpipc_buffer_create shmat. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Initialize the buffer struct  */
    memset(buf, 0, bufsize);
    buf->length = length;
    buf->boundry = length-1;
    buf->range = length;
    buf->channels = channels;
    buf->samplerate = samplerate;

    /* Create the semaphore used as a read/write lock on the buffer */
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid < 0) {
        syslog(LOG_ERR, "lpipc_buffer_create semget. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Prime the lock by initalizing the sempahore to 1 */
    arg.val = 1;
    if(semctl(semid, 0, SETVAL, arg) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_create semctl. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Write the identifier for the sempahore into a 
     * well known file other processes can use to attach and read */
    sempath_size = snprintf(NULL, 0, "%s_semid", id_path);
    sempath = (char *)calloc(1, sempath_size+1);
    if(snprintf(sempath, sempath_size, "%s_semid", id_path) < 0) {
        syslog(LOG_ERR, "lpadc_create failed to concat sempath %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    if(lpipc_setid(sempath, semid) < 0) {
        syslog(LOG_ERR, "lpadc_create failed to store shared memory ID to path %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    free(sempath);

    return 0;
}

int lpipc_buffer_aquire(char * id_path, lpipc_buffer_t ** buf) {
    int shmid, semid;
    struct sembuf sop;
    char * sempath;
    size_t sempath_size;


    /* Construct the sempahore ID path */
    sempath_size = snprintf(NULL, 0, "%s_semid", id_path);
    sempath = (char *)calloc(1, sempath_size+1);
    if(snprintf(sempath, sempath_size, "%s_semid", id_path) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_aquire failed to concat sempath %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    /* Look up the sempahore ID */
    if((semid = lpipc_getid(sempath)) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_aquire Could not get semid %s. Error: %s\n", sempath, strerror(errno));
        return -1;
    }

    free(sempath);

    /* Look up the shared memory ID */
    if((shmid = lpipc_getid(id_path)) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_aquire Could not get shmid %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    /* Aquire a lock on the semaphore */
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if(semop(semid, &sop, 1) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_aquire semop. Could not aquire sem lock. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Attach the shared memory to the pointer */
    *buf = (lpipc_buffer_t *)shmat(shmid, NULL, 0);
    if(*buf == (void *)-1) {
        syslog(LOG_ERR, "lpipc_buffer_aquire shmat. Could not attach to shm. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int lpipc_buffer_release(char * id_path) {
    int semid;
    struct sembuf sop;
    char * sempath;
    size_t sempath_size;


    /* Construct the sempahore ID path */
    sempath_size = snprintf(NULL, 0, "%s_semid", id_path);
    sempath = (char *)calloc(1, sempath_size+1);
    if(snprintf(sempath, sempath_size, "%s_semid", id_path) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release failed to concat sempath %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    /* Look up the sempahore ID */
    if((semid = lpipc_getid(sempath)) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release Could not get semid %s. Error: %s\n", sempath, strerror(errno));
        return -1;
    }

    /* Release lock */
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if(semop(semid, &sop, 1) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release semop. Error: %s\n", strerror(errno));
        return -1;
    }

    free(sempath);
    return 0;
}

int lpipc_buffer_tolpbuffer(lpipc_buffer_t * ipcbuf, lpbuffer_t ** buf) {
    size_t bufsize;
    *buf = LPBuffer.create(ipcbuf->length, ipcbuf->channels, ipcbuf->samplerate);
    bufsize = sizeof(lpfloat_t) * (*buf)->length * (*buf)->channels;
    memcpy((*buf)->data, (lpfloat_t *)ipcbuf->data, bufsize);
    return 0;
}

int lpipc_buffer_destroy(char * id_path) {
    int shmid, semid;
    union semun dummy;
    char * sempath;
    size_t sempath_size;


    /* Construct the sempahore ID path */
    sempath_size = snprintf(NULL, 0, "%s_semid", id_path);
    sempath = (char *)calloc(1, sempath_size+1);
    if(snprintf(sempath, sempath_size, "%s_semid", id_path) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release failed to concat sempath %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    /* Look up the sempahore ID */
    if((semid = lpipc_getid(sempath)) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release Could not get semid %s. Error: %s\n", sempath, strerror(errno));
        return -1;
    }
    free(sempath);

    /* Look up the shared memory ID */
    if((shmid = lpipc_getid(id_path)) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release Could not get shmid %s. Error: %s\n", id_path, strerror(errno));
        return -1;
    }

    /* Remove the semaphore and shared memory buffer */
    if(shmctl(shmid, IPC_RMID, NULL) < 0) {
        syslog(LOG_ERR, "lpcounter_destroy shmctl. Error: %s\n", strerror(errno));
        return -1;
    }

    if(semctl(semid, 0, IPC_RMID, dummy) < 0) {
        syslog(LOG_ERR, "lpcounter_destroy semctl. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int lpadc_getid() {
    return lpipc_getid(LPADC_HANDLE);
}

int lpadc_create() {
    int shmid;
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
        syslog(LOG_ERR, "lpadc_create shmget. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Get info about the shared memory... but why? */
    if(shmctl(shmid, IPC_STAT, &shm_info) == -1) {
        syslog(LOG_ERR, "lpadc_create shmctl. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Write the identifier for the adcbuf shared memory into a 
     * well known file other processes can use to attach and read */
    if(lpipc_setid(LPADC_HANDLE, shmid) < 0) {
        syslog(LOG_ERR, "lpadc_create failed to store shared memory ID to path %s. Error: %s\n", LPADC_HANDLE, strerror(errno));
        return -1;
    }

    /* Attach the shared memory to adcbuf */
    adcbuf = (lpadcbuf_t *)shmat(shmid, NULL, 0);
    if(adcbuf == (void *)-1) {
        syslog(LOG_ERR, "lpadc_create shmat. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Initialize the shared memory as a buffer */
    adc.pos = 0;
    memset(adc.buf, 0, sizeof(adc.buf));
    memcpy(adcbuf, &adc, sizeof(lpadcbuf_t));

    return 0;
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
        syslog(LOG_ERR, "lpadc_destroy shmctl: Could not destroy adcbuf shared memory. Error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/* MIDI STATUS IPC
 * GETTERS & SETTERS
 * ****************/
int lpmidi_setcc(int device_id, int cc, int value) {
    char * cc_path;
    size_t path_length;

    path_length = snprintf(NULL, 0, ASTRID_MIDI_CCBASE_PATH, device_id, cc) + 1;
    cc_path = (char *)calloc(1, path_length);
    snprintf(cc_path, path_length,  ASTRID_MIDI_CCBASE_PATH, device_id, cc);

    if(lpipc_setid(cc_path, value) < 0) {
        syslog(LOG_ERR, "Could not store %d for MIDI CC %d from device %d\n", value, cc, device_id);
        return -1;
    }

    free(cc_path);

    return 0;
}

int lpmidi_getcc(int device_id, int cc) {
    char * cc_path;
    size_t path_length;
    int value;

    path_length = snprintf(NULL, 0, ASTRID_MIDI_CCBASE_PATH, device_id, cc) + 1;
    cc_path = (char *)calloc(1, path_length);
    snprintf(cc_path, path_length,  ASTRID_MIDI_CCBASE_PATH, device_id, cc);

    value = lpipc_getid(cc_path);
    if(value < 0) {
        value = 0;
        lpipc_setid(cc_path, value); 
    }

    return value;
}

int lpmidi_setnote(int device_id, int note, int velocity) {
    char * note_path;
    size_t path_length;

    path_length = snprintf(NULL, 0, ASTRID_MIDI_NOTEBASE_PATH, device_id, note) + 1;
    note_path = (char *)calloc(1, path_length);
    snprintf(note_path, path_length,  ASTRID_MIDI_NOTEBASE_PATH, device_id, note);

    if(lpipc_setid(note_path, velocity) < 0) {
        syslog(LOG_ERR, "Could not store velocity %d for MIDI note %d from device %d\n", velocity, note, device_id);
        return -1;
    }

    free(note_path);

    return 0;
}

int lpmidi_getnote(int device_id, int note) {
    char * note_path;
    size_t path_length;
    int value;

    path_length = snprintf(NULL, 0, ASTRID_MIDI_NOTEBASE_PATH, device_id, note) + 1;
    note_path = (char *)calloc(1, path_length);
    snprintf(note_path, path_length,  ASTRID_MIDI_NOTEBASE_PATH, device_id, note);

    value = lpipc_getid(note_path);
    if(value < 0) {
        value = 0;
        lpipc_setid(note_path, value); 
    }

    return value;
}

/* MIDI trigger maps for noteon 
 * (and eventually cc triggers)
 * ***************************/

int lpmidi_add_msg_to_notemap(int device_id, int note, lpmsg_t msg) {
    int fd, bytes_written;
    size_t notemap_path_length;
    char * notemap_path;

    notemap_path_length = snprintf(NULL, 0, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note) + 1;
    notemap_path = (char *)calloc(1, notemap_path_length);
    snprintf(notemap_path, notemap_path_length, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note);

    if((fd = open(notemap_path, O_RDWR | O_CREAT | O_APPEND, 0777)) < 0) {
        syslog(LOG_ERR, "Could not open file for appending. Error: %s\n", strerror(errno));
        return -1;
    }

    if((bytes_written = write(fd, &msg, sizeof(lpmsg_t))) < 0) {
        syslog(LOG_ERR, "Could not append msg to file. Error: %s\n", strerror(errno));
        return -1;
    }

    if(bytes_written != sizeof(lpmsg_t)) {
        syslog(LOG_ERR, "Wrote incorrect number of bytes to notemap. Expected %d, wrote %d\n", (int)sizeof(lpmsg_t), bytes_written);
        return -1;
    }

    if(close(fd) < 0) {
        syslog(LOG_ERR, "Could not close notemap file after writing. Error: %s\n", strerror(errno));
        return -1;
    }

    free(notemap_path);

    return 0;
}

int lpmidi_remove_msg_from_notemap(int device_id, int note, int index_to_remove) {
    int fd, bytes_read, bytes_written, map_index;
    struct stat statbuf;
    size_t notemap_path_length, notemap_size, read_pos;
    char * notemap_path;
    lpmsg_t msg = {0};

    notemap_path_length = snprintf(NULL, 0, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note) + 1;
    notemap_path = (char *)calloc(1, notemap_path_length);
    snprintf(notemap_path, notemap_path_length, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note);

    if((fd = open(notemap_path, O_RDWR)) < 0) {
        syslog(LOG_ERR, "Could not open file for printing. Error: %s\n", strerror(errno));
        return -1;
    }

    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "Could not stat notemap file. Error: %s\n", strerror(errno));
        return -1;
    }

    notemap_size = statbuf.st_size;
    map_index = 0;
    for(read_pos=0; read_pos < notemap_size; read_pos += sizeof(lpmsg_t)) {
        if((bytes_read = read(fd, &msg, sizeof(lpmsg_t))) < 0) {
            syslog(LOG_ERR, "Could not read msg from file during removal walk. Error: %s\n", strerror(errno));
            return -1;
        }

        if(map_index == index_to_remove) {
            msg.type = LPMSG_EMPTY;

            if(lseek(fd, read_pos, SEEK_SET) < 0) {
                syslog(LOG_ERR, "Could not rewind midimap to remove msg from file. Error: %s\n", strerror(errno));
                return -1;
            }

            if((bytes_written = write(fd, &msg, sizeof(lpmsg_t))) < 0) {
                syslog(LOG_ERR, "Could not remove msg from file. Error: %s\n", strerror(errno));
                return -1;
            }
        }

        map_index += 1;
    }

    if(close(fd) < 0) {
        syslog(LOG_ERR, "Could not close notemap file after msg removal. Error: %s\n", strerror(errno));
        return -1;
    }

    free(notemap_path);

    return 0;
}

int lpmidi_print_notemap(int device_id, int note) {
    int fd, bytes_read, map_index;
    struct stat statbuf;
    size_t notemap_path_length, notemap_size, read_pos;
    char * notemap_path;
    lpmsg_t msg = {0};

    notemap_path_length = snprintf(NULL, 0, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note) + 1;
    notemap_path = (char *)calloc(1, notemap_path_length);
    snprintf(notemap_path, notemap_path_length, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note);

    if((fd = open(notemap_path, O_RDONLY)) < 0) {
        syslog(LOG_ERR, "Could not open file for printing. Error: %s\n", strerror(errno));
        return -1;
    }

    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "Could not stat notemap file. Error: %s\n", strerror(errno));
        return -1;
    }

    notemap_size = statbuf.st_size;
    map_index = 0;
    for(read_pos=0; read_pos < notemap_size; read_pos += sizeof(lpmsg_t)) {
        if((bytes_read = read(fd, &msg, sizeof(lpmsg_t))) < 0) {
            syslog(LOG_ERR, "Could not read msg from file. Error: %s\n", strerror(errno));
            return -1;
        }

        printf("\nmap_index: %d\n", map_index);
        printf("bytes_read: %d, read_pos: %ld, notemap_size: %ld\n", bytes_read, read_pos, notemap_size);
        printf("msg.type: %d msg.timestamp: %f msg.instrument_name: %s\n", msg.type, msg.timestamp, msg.instrument_name);
        if(msg.type == LPMSG_EMPTY) {
            printf("this message is empty!\n");
        }

        map_index += 1;
    }

    if(close(fd) < 0) {
        syslog(LOG_ERR, "Could not close notemap file after reading. Error: %s\n", strerror(errno));
        return -1;
    }

    free(notemap_path);

    return 0;
}

int lpmidi_trigger_notemap(int device_id, int note) {
    int fd, bytes_read;
    struct stat statbuf;
    size_t notemap_path_length, notemap_size, read_pos;
    char * notemap_path;
    lpmsg_t msg = {0};

    notemap_path_length = snprintf(NULL, 0, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note) + 1;
    notemap_path = (char *)calloc(1, notemap_path_length);
    snprintf(notemap_path, notemap_path_length, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note);

    if((fd = open(notemap_path, O_RDWR)) < 0) {
        syslog(LOG_ERR, "Could not open file for printing. Error: %s\n", strerror(errno));
        return -1;
    }

    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "Could not stat notemap file. Error: %s\n", strerror(errno));
        return -1;
    }

    notemap_size = statbuf.st_size;
    for(read_pos=0; read_pos < notemap_size; read_pos += sizeof(lpmsg_t)) {
        if((bytes_read = read(fd, &msg, sizeof(lpmsg_t))) < 0) {
            syslog(LOG_ERR, "Could not read msg from file during notemap trigger walk. Error: %s\n", strerror(errno));
            return -1;
        }

        if(msg.type == LPMSG_EMPTY) continue;

        syslog(LOG_INFO, "Sending message from lpmidi trigger notemap\n");
        if(send_message(msg) < 0) {
            syslog(LOG_ERR, "Could not schedule msg for sending during notemap trigger. Error: %s\n", strerror(errno));
            return -1;
        }
        syslog(LOG_INFO, "Sent! lpmidi trigger notemap message\n");
    }

    if(close(fd) < 0) {
        syslog(LOG_ERR, "Could not close notemap file after triggering. Error: %s\n", strerror(errno));
        return -1;
    }

    free(notemap_path);

    return 0;
}


/* BUFFER
 * SERIALIZATION
 * *************/
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

/* MESSAGE
 * QUEUES
 * ******/
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
        syslog(LOG_ERR, "get_play_message mkfifo: Error creating named pipe. Error: %s\n", strerror(errno));
        return -1;
    }

    qfd = open(qname, O_RDONLY);
    if((read_result = read(qfd, msg, sizeof(lpmsg_t))) != sizeof(lpmsg_t)) {
        syslog(LOG_INFO, "Play queue for %s has closed\n", instrument_name);
        return -1;
    }

    if(close(qfd) == -1) {
        syslog(LOG_ERR, "get_play_message mkfifo: Error closing q. Error: %s\n", strerror(errno));
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

    umask(0);
    if(mkfifo(qname, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        syslog(LOG_ERR, "astrid_playq_open mkfifo: Error creating play queue FIFO. Error: %s\n", strerror(errno));
        return -1;
    }

    if((qfd = open(qname, O_RDWR)) < 0) {
        syslog(LOG_ERR, "astrid_playq_open open: Error opening play queue FIFO. Error: %s\n", strerror(errno));
        return -1;
    };

    return qfd;
}

int astrid_playq_close(int qfd) {
    if(close(qfd) == -1) {
        syslog(LOG_ERR, "astrid_playq_close close: Error closing play queue FIFO. Error: %s\n", strerror(errno));
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
    syslog(LOG_DEBUG, " Q           %f (msg.timestamp)\n", msg->timestamp);

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
        syslog(LOG_ERR, "send_play_message mkfifo: Error creating named pipe. Error: %s\n", strerror(errno));
        return -1;
    }

    qfd = open(qname, O_WRONLY);

    if(write(qfd, &msg, sizeof(lpmsg_t)) != sizeof(lpmsg_t)) {
        syslog(LOG_ERR, "send_play_message write: Could not write to q. Error: %s\n", strerror(errno));
        return -1;
    }

    if(close(qfd) == -1) {
        syslog(LOG_ERR, "send_play_message close: Error closing play q. Error: %s\n", strerror(errno));
        return -1; 
    }

    syslog(LOG_DEBUG, " P           %s MSG OUT %s\n", msg.instrument_name, msg.msg);
    syslog(LOG_DEBUG, " P           %s (msg.instrument_name)\n", msg.instrument_name);
    syslog(LOG_DEBUG, " P           %d (msg.voice_id)\n", (int)msg.voice_id);
    syslog(LOG_DEBUG, " P           %f (msg.timestamp)\n", msg.timestamp);


    return 0;
}

int astrid_msgq_open() {
    int qfd;

    umask(0);
    if(mkfifo(ASTRID_MSGQ_PATH, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        syslog(LOG_ERR, "astrid_msgq_open mkfifo: Error creating msg queue FIFO. Error: %s\n", strerror(errno));
        return -1;
    }

    if((qfd = open(ASTRID_MSGQ_PATH, O_RDWR)) < 0) {
        syslog(LOG_ERR, "astrid_msgq_open open: Error opening msg queue FIFO. Error: %s\n", strerror(errno));
        return -1;
    };

    return qfd;
}

int astrid_msgq_close(int qfd) {
    if(close(qfd) == -1) {
        syslog(LOG_ERR, "astrid_msgq_close close: Error closing msg queue FIFO. Error: %s\n", strerror(errno));
        return -1; 
    }

    return 0;
}

int astrid_msgq_read(int qfd, lpmsg_t * msg) {
    ssize_t read_result;
    read_result = read(qfd, msg, sizeof(lpmsg_t));
    if(read_result == 0) {
        syslog(LOG_DEBUG, "The msg queue (%d) has been closed. (EOF)\n", qfd);
        return -1;
    }

    if(read_result != sizeof(lpmsg_t)) {
        syslog(LOG_INFO, "The msg queue (%d) returned %d bytes. Expecting sizeof(lpmsg_t)==%d\n", qfd, (int)read_result, (int)sizeof(lpmsg_t));
        return -1;
    }

    syslog(LOG_DEBUG, " MQ           %s MSG READ FROM Q %d\n", msg->instrument_name, qfd);
    syslog(LOG_DEBUG, " MQ           %s (msg.instrument_name)\n", msg->instrument_name);
    syslog(LOG_DEBUG, " MQ           %d (msg.voice_id)\n", (int)msg->voice_id);
    syslog(LOG_DEBUG, " MQ           %f (msg.timestamp)\n", msg->timestamp);

    return 0;
}

int parse_message_from_args(int argc, int arg_offset, char * argv[], lpmsg_t * msg) {
    int bytesread, a, i, length, voice_id;
    char msgtype;
    char message_params[LPMAXMSG] = {0};
    char instrument_name[LPMAXNAME] = {0};
    size_t instrument_name_length;
    lpcounter_t c;

    msgtype = argv[arg_offset + 1][0];

    /* Prepare the message param string */
    bytesread = 0;
    for(a=arg_offset+2; a < argc; a++) {
        length = strlen(argv[a]);
        if(a==arg_offset+2) {
            instrument_name_length = (size_t)length;
            memcpy(instrument_name, argv[a], instrument_name_length);
            continue;
        }

        for(i=0; i < length; i++) {
            message_params[bytesread] = argv[a][i];
            bytesread++;
        }
        message_params[bytesread] = SPACE;
        bytesread++;
    }

    /* Set up the message struct */
    strncpy(msg->instrument_name, instrument_name, instrument_name_length);
    strncpy(msg->msg, message_params, bytesread);

    /* Set the message type from the first arg */
    switch(msgtype) {
        case PLAY_MESSAGE:
            msg->type = LPMSG_PLAY;
            break;

        case TRIGGER_MESSAGE:
            msg->type = LPMSG_TRIGGER;
            break;

        case LOAD_MESSAGE:
            msg->type = LPMSG_LOAD;
            break;

        case STOP_MESSAGE:
            msg->type = LPMSG_STOP;
            break;

        case SHUTDOWN_MESSAGE:
            msg->type = LPMSG_SHUTDOWN;
            break;

        default:
            msg->type = LPMSG_SHUTDOWN;
            break;
    }

    /* Get the voice ID from the voice ID counter */
    if((c.semid = lpipc_getid(LPVOICE_ID_SEMID)) < 0) {
        syslog(LOG_ERR, "Problem trying to get voice ID sempahore handle when constructing play message from command input\n");
        return -1;
    }

    if((c.shmid = lpipc_getid(LPVOICE_ID_SHMID)) < 0) {
        syslog(LOG_ERR, "Problem trying to get voice ID shared memory handle when constructing play message from command input\n");
        return -1;
    }

    if((voice_id = lpcounter_read_and_increment(&c)) < 0) {
        syslog(LOG_ERR, "Problem trying to get voice ID when constructing play message from command input\n");
        return -1;
    }

    msg->voice_id = voice_id;

    /* Initialize the count */
    msg->count = 0;

    return 0;
}

int send_message(lpmsg_t msg) {
    int qfd;

    syslog(LOG_ERR, "send_message SENDING MSG ts         %f\n", msg.timestamp);
    syslog(LOG_ERR, "send_message SENDING MSG instrument %s\n", msg.instrument_name);

    syslog(LOG_DEBUG, "SM mkfifo\n");

    umask(0);
    if(mkfifo(ASTRID_MSGQ_PATH, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        syslog(LOG_ERR, "send_message mkfifo: Error creating named pipe. Error: %s\n", strerror(errno));
        return -1;
    }

    syslog(LOG_DEBUG, "SM open\n");
    if((qfd = open(ASTRID_MSGQ_PATH, O_WRONLY)) < 0) {
        syslog(LOG_ERR, "send_message open: Could not open q. Error: %s\n", strerror(errno));
    }

    syslog(LOG_DEBUG, "SM write\n");
    if(write(qfd, &msg, sizeof(lpmsg_t)) != sizeof(lpmsg_t)) {
        syslog(LOG_ERR, "send_message write: Could not write to q. Error: %s\n", strerror(errno));
        return -1;
    }

    syslog(LOG_DEBUG, "SM close\n");
    if(close(qfd) == -1) {
        syslog(LOG_ERR, "send_message close: Error closing play q. Error: %s\n", strerror(errno));
        return -1; 
    }

    syslog(LOG_DEBUG, " SM           %s MSG OUT %s\n", msg.instrument_name, msg.msg);
    syslog(LOG_DEBUG, " SM           %s (msg.instrument_name)\n", msg.instrument_name);
    syslog(LOG_DEBUG, " SM           %d (msg.voice_id)\n", (int)msg.voice_id);
    syslog(LOG_DEBUG, " SM           %f (msg.timestamp)\n", msg.timestamp);

    return 0;
}

int midi_triggerq_open() {
    int qfd;

    umask(0);
    if(mkfifo(ASTRID_MIDI_TRIGGERQ_PATH, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST) {
        syslog(LOG_ERR, "midi_triggerq_open mkfifo: Error creating msg queue FIFO. Error: %s\n", strerror(errno));
        return -1;
    }

    if((qfd = open(ASTRID_MIDI_TRIGGERQ_PATH, O_RDWR)) < 0) {
        syslog(LOG_ERR, "midi_triggerq_open open: Error opening msg queue FIFO. Error: %s\n", strerror(errno));
        return -1;
    };

    return qfd;
}

int midi_triggerq_schedule(int qfd, lpmidievent_t t) {
    if(write(qfd, &t, sizeof(lpmidievent_t)) != sizeof(lpmidievent_t)) {
        syslog(LOG_ERR, "midi_triggerq_schedule write: Could not write to q. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int midi_triggerq_close(int qfd) {
    if(close(qfd) == -1) {
        syslog(LOG_ERR, "midi_triggerq_close close: Error closing msg queue FIFO. Error: %s\n", strerror(errno));
        return -1; 
    }

    return 0;
}


/* SCHEDULING
 * and MIXING
 * **********/
int lpscheduler_get_now_seconds(double * now) {
    clockid_t cid;
    struct timespec ts;

#if defined(__linux__)
    cid = CLOCK_MONOTONIC_RAW;
#else
    cid = CLOCK_MONOTONIC;
#endif

    if(clock_gettime(cid, &ts) < 0) {
        syslog(LOG_ERR, "scheduler_get_now_seconds: clock_gettime error: %s\n", strerror(errno));
        return -1; 
    }

    *now = ts.tv_sec + ts.tv_nsec * 1e-9;

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
        syslog(LOG_CRIT, "Cannot move this event. There is nothing in the waiting queue!\n");
        return;
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

void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay) {
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

