#include "astrid.h"

int init_instrument_message(lpmsg_t * msg, char * instrument_name);

static volatile int * astrid_instrument_is_running;
static pthread_mutex_t astrid_nursery_lock = PTHREAD_MUTEX_INITIALIZER;

void handle_instrument_shutdown(__attribute__((unused)) int sig) {
    *astrid_instrument_is_running = 0;
}

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
static int lpsessiondb_callback_debug(__attribute__((unused)) void * unused, int argc, char ** argv, char ** colname) {
    int i;
    for(i=0; i < argc; i++) {
        syslog(LOG_DEBUG, "lpsessiondb_callback %s=%s\n", colname[i], argv[i] ? argv[i] : "None");
    }
    return 0;
}

static int __attribute__((unused)) lpsessiondb_callback_noop(__attribute__((unused)) void * unused, __attribute__((unused)) int argc, __attribute__((unused)) char ** argv, __attribute__((unused)) char ** colname) {
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
    sqlsize = snprintf(NULL, 0, _sql, now, msg.initiated, (int)msg.voice_id, msg.instrument_name, msg.msg)+1;
    sql = calloc(1, sqlsize);
    if(snprintf(sql, sqlsize, _sql, now, msg.initiated, (int)msg.voice_id, msg.instrument_name, msg.msg) < 0) {
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
    sqlsize = snprintf(NULL, 0, "update voices set active=1, started=%lld, last_render=%lld, render_count=1 where id=%d;", now, now, voice_id)+1;
    sql = calloc(1, sqlsize);
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

    if(count == SIZE_MAX) {
        count = 0;
    }

    sqlsize = snprintf(NULL, 0, "update voices set active=1, last_render=%lld, render_count=%ld where id=%d;", now, count, voice_id)+1;
    if((sql = calloc(1, sqlsize)) == NULL) {
        syslog(LOG_ERR, "lpsessiondb_increment_voice_render_count Could not alloc space for sql query. Error: (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    if(snprintf(sql, sqlsize, "update voices set active=1, last_render=%lld, render_count=%ld where id=%d;", now, count, voice_id) < 0) {
        syslog(LOG_ERR, "lpsessiondb_increment_voice_render_count Could not concat sql for update. Error: (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    /* Mark the voice as active */
    if(sqlite3_exec(db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "lpsessiondb_increment_voice_render_count Could not exec sql statement: %s. Error: (%d) %s\n", sql, errno, strerror(errno));
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
    sqlsize = snprintf(NULL, 0, "update voices set active=0, ended=%lld, last_render=%lld, render_count=%ld where id=%d;", now, now, count, voice_id)+1;
    if((sql = calloc(1, sqlsize)) == NULL) {
        syslog(LOG_ERR, "lpsessiondb_mark_voice_stopped Could not alloc space for sql query. Error: (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    if(snprintf(sql, sqlsize, "update voices set active=0, ended=%lld, last_render=%lld, render_count=%ld where id=%d;", now, now, count, voice_id) < 0) {
        syslog(LOG_ERR, "lpsessiondb_mark_voice_stopped Could not concat sql for update. Error: (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    /* Mark the voice as stopped */
    if(sqlite3_exec(db, sql, lpsessiondb_callback_debug, 0, &err) != SQLITE_OK) {
        syslog(LOG_ERR, "lpsessiondb_mark_voice_stopped Could not exec sql statement: %s. Error: (%d) %s\n", sql, errno, strerror(errno));
        return -1;
    }

    return 0;
}

#endif

/* SHARED MEMORY
 * COMMUNICATION TOOLS
 * *******************/
// FIXME -- these get/set ID calls should be replaced with an LMDB 
// backend which manages a global session of settings for all instruments
int lpipc_setid(char * path, int id) {
    char val[20];
    int fd;

    if((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpipc_setid open: %s. Could not open id file for locking. Error: %s\n", path, strerror(errno));
        return -1;
    }

    if(flock(fd, LOCK_EX) < 0) {
        syslog(LOG_ERR, "lpipc_setid flock: %s. Could not lock id file. Error: %s\n", path, strerror(errno));
        return -1;
    }

    snprintf(val, sizeof(val), "%d", id);

    if(write(fd, val, strlen(val)) < 0) {
        syslog(LOG_ERR, "lpipc_setid write: %s. Could not write to id file. Error: %s\n", path, strerror(errno));
        return -1;
    }

    if(flock(fd, LOCK_UN) < 0) {
        syslog(LOG_ERR, "lpipc_setid flock: %s. Could not unlock id file. Error: %s\n", path, strerror(errno));
        return -1;
    }

    close(fd);

    return 0;
}

int lpipc_lockid(char * path) {
    int fd;
    if((fd = open(path, O_RDWR)) < 0) {
        syslog(LOG_ERR, "lpipc_lockid open: %s. Could not open id file for locking. Error: %s\n", path, strerror(errno));
        return -1;
    }

    if(flock(fd, LOCK_EX) < 0) {
        syslog(LOG_ERR, "lpipc_lockid flock: %s. Could not lock id file. Error: %s\n", path, strerror(errno));
        return -1;
    }

    return fd;
}

int lpipc_unlockid(char * path) {
    int fd;
    if((fd = open(path, O_RDWR)) < 0) {
        syslog(LOG_ERR, "lpipc_lockid open: %s. Could not open id file for locking. Error: %s\n", path, strerror(errno));
        return -1;
    }

    if(flock(fd, LOCK_UN) < 0) {
        syslog(LOG_ERR, "lpipc_lockid flock: %s. Could not lock id file. Error: %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

int lpipc_getid(char * path) {
    int fd, id = 0;
    /*char * idp;*/
    struct stat st;
    int bytes_read;
    char val[20];

    if(access(path, F_OK) != 0) {
        if(lpipc_setid(path, 0) < 0) {
            syslog(LOG_ERR, "lpipc_getid could not write new id file: %s. Error: %s\n", path, strerror(errno));
            return -1;
        }
        return 0;
    }

    /* Read the identifier from the well known file. mmap speeds 
     * up access when ids need to be fetched when timing matters */
    fd = open(path, O_RDONLY);
    if(fd < 0) {
        syslog(LOG_ERR, "lpipc_getid could not open path: %s. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    if(flock(fd, LOCK_EX) < 0) {
        syslog(LOG_ERR, "lpipc_getid flock: %s. Could not lock id file. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    if(fstat(fd, &st) == -1) {
        syslog(LOG_ERR, "lpipc_getid fstat: %s. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    if((bytes_read = read(fd, val, st.st_size)) < 0) {
        syslog(LOG_ERR, "lpipc_getid read: %s. Could not read from id file. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    val[bytes_read] = '\0';
    id = atoi(val);

    if(flock(fd, LOCK_UN) < 0) {
        syslog(LOG_ERR, "lpipc_setid flock: %s. Could not unlock id file. Error: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    return id;
}

int lpipc_destroyid(char * path) {
    if(unlink(path) < 0) {
        syslog(LOG_ERR, "lpipc_destroyid unlink: %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

int lpipc_createvalue(char * path, size_t size) {
    int shmfd;
    char * semname;

    /* Construct the sempahore name by stripping the /tmp prefix */
    semname = path + 4;

    /* Create the POSIX semaphore and initialize it to 1 */
    if(sem_open(semname, O_CREAT | O_EXCL, LPIPC_PERMS, 1) == NULL) {
        syslog(LOG_ERR, "lpipc_createvalue failed to create semaphore %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Create the POSIX shared memory segment */
    if((shmfd = shm_open(semname, O_CREAT | O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpipc_createvalue Could not create shared memory segment. (%s) %s\n", semname, strerror(errno));
        return -1;
    }

    if(ftruncate(shmfd, size) < 0) {
        syslog(LOG_ERR, "lpipc_createvalue Could not truncate shared memory segment to size %ld. (%s) %s\n", size, semname, strerror(errno));
        return -1;
    }

    close(shmfd);

    return 0;
}

int lpipc_setvalue(char * path, void * value, size_t size) {
    int fd;
    sem_t * sem;
    void * shmaddr;
    char * semname;

    /* Construct the sempahore name by stripping the /tmp prefix */
    /* FIXME move path prefixes to config */
    semname = path + 4;

    /* Open the semaphore */
    if((sem = sem_open(semname, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpipc_setvalue failed to open semaphore %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Aquire a lock on the semaphore */
    if(sem_wait(sem) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue failed to decrementsem %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Get the file descriptor for the shared memory segment */
    if((fd = shm_open(semname, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue Could not open shared memory segment. (%s) %s\n", semname, strerror(errno));
        return -1;
    }

    /* Attach the shared memory to the pointer */
    if((shmaddr = (void*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_setvalue Could not mmap shared memory segment to size %ld. (%s) %s\n", size, semname, strerror(errno));
        return -1;
    }

    /* Write the value into the shared memory segment */
    memcpy(shmaddr, value, size);

    /* unmap the shared memory... */
    munmap(shmaddr, size);

    /* Release the lock on the semaphore */
    if(sem_post(sem) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue failed to unlock %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }
    
    /* Clean up sempahore resources */
    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue sem_close Could not close semaphore\n");
        return -1;
    }

    close(fd);

    return 0;
}

int lpipc_unsafe_getvalue(char * path, void ** value) {
    struct stat statbuf;
    void * shmaddr;
    int fd;

    /* Get the file descriptor for the shared memory segment */
    if((fd = shm_open(path+4, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpipc_unsafe_getvalue Could not open shared memory segment. (%s) %s\n", path, strerror(errno));
        return -1;
    }

    /* Get the size of the segment */
    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "lpipc_unsafe_getvalue Could not stat shm. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Attach the shared memory to the pointer */
    if((shmaddr = (void*)mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_unsafe_getvalue Could not mmap shared memory segment to size %ld. (%s) %s\n", statbuf.st_size, path, strerror(errno));
        return -1;
    }

    memcpy(*value, shmaddr, statbuf.st_size);

    /* unmap the shared memory... */
    munmap(shmaddr, statbuf.st_size);

    close(fd);

    return 0;
}

int lpipc_getvalue(char * path, void ** value) {
    struct stat statbuf;
    int fd;
    sem_t * sem;
    void * shmaddr;
    char * semname;

    /* Construct the sempahore name by stripping the /tmp prefix */
    semname = path + 4;

    /* Open the semaphore */
    if((sem = sem_open(semname, 0, LPIPC_PERMS)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpipc_setvalue failed to open semaphore %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Aquire a lock on the semaphore */
    if(sem_wait(sem) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue failed to decrementsem %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Get the file descriptor for the shared memory segment */
    if((fd = shm_open(semname, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpipc_getvalue Could not open shared memory segment. (%s) %s\n", semname, strerror(errno));
        return -1;
    }

    /* Get the size of the segment */
    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "lpipc_getvalue Could not stat shm. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Attach the shared memory to the pointer */
    if((shmaddr = (void*)mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_getvalue Could not mmap shared memory segment to size %ld. (%s) %s\n", statbuf.st_size, semname, strerror(errno));
        return -1;
    }

    memcpy(*value, shmaddr, statbuf.st_size);

    /* unmap the shared memory... */
    munmap(shmaddr, statbuf.st_size);

    /* Clean up sempahore resources */
    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpipc_getvalue sem_close Could not close semaphore\n");
        return -1;
    }

    close(fd);

    return 0;
}

int lpipc_releasevalue(char * id_path) {
    sem_t * sem;
    char * semname;

    /* Construct the sempahore name by stripping the /tmp prefix */
    semname = id_path + 4;

    /* Open the semaphore */
    if((sem = sem_open(semname, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpipc_releasevalue failed to open semaphore %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Release the lock on the semaphore */
    if(sem_post(sem) < 0) {
        syslog(LOG_ERR, "lpipc_releasevalue failed to unlock %s. Error: %s\n", semname, strerror(errno));
        return -1;
    }

    /* Clean up sempahore resources */
    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpipc_releasevalue sem_close Could not close semaphore\n");
        return -1;
    }

    return 0;
}

int lpipc_destroyvalue(char * path) {
    char * semname;

    semname = path + 4;

    if(sem_unlink(semname) < 0) {
        syslog(LOG_ERR, "lpipc_destroyvalue sem_unlink Could not destroy semaphore\n");
        return -1;
    }

    return 0;
}


/* SHARED MEMORY
 * BUFFER TOOLS
 * ************/
int lpsampler_get_path(char * name, char * path) {
    snprintf(path, PATH_MAX, "/astrid-sampler-%s", name);
    return 0;
}

lpbuffer_t * lpsampler_create(char * name, double length_in_seconds, int channels, int samplerate) {
    int shmfd;
    sem_t * sem;
    lpbuffer_t * buf;
    size_t bufsize;
    char path[PATH_MAX] = {0};
    size_t length = (size_t)(length_in_seconds * samplerate);

    lpsampler_get_path(name, path);

    /* Determine the size of the shared memory segment */
    bufsize = sizeof(lpbuffer_t) + (length * channels * sizeof(lpfloat_t));
    syslog(LOG_DEBUG, "bufsize=%ld samplerate=%d length=%ld channels=%d\n", 
            bufsize, samplerate, length, channels);

    /* Create the POSIX semaphore and initialize it to 1 */
    if((sem = sem_open(path, O_CREAT, LPIPC_PERMS, 1)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpipc_buffer_create Could not create semaphore. (%s) %s\n", path, strerror(errno));
        return NULL;
    }

    /* initialize the shared memory segment for the lpbuffer_t struct */
    if((shmfd = shm_open(path, O_CREAT | O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "Could not create shared memory segment. (%s) %s\n", path, strerror(errno));
        return NULL; 
    }

    if(ftruncate(shmfd, bufsize) < 0) {
        syslog(LOG_ERR, "Could not truncate shared memory segment to size %ld. (%s) %s\n", sizeof(lpbuffer_t), path, strerror(errno));
        return NULL;
    }

    if((buf = mmap(NULL, bufsize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "Could not mmap shared memory segment to size %ld. (%s) %s\n", sizeof(lpbuffer_t), path, strerror(errno));
        return NULL;
    }

    memset(buf, 0, bufsize);
    buf->channels = channels;
    buf->length = length;
    buf->samplerate = samplerate;
    buf->boundry = length-1;
    buf->range = length;

    close(shmfd);

    return buf;
}

lpbuffer_t * lpsampler_aquire_and_map(char * name) {
    struct stat statbuf;
    int fd;
    lpbuffer_t * buf;
    sem_t * sem;
    char path[PATH_MAX] = {0};
    lpsampler_get_path(name, path);

    /* Open the semaphore */
    if((sem = sem_open(path, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpsampler_aquire_and_map failed to open semaphore %s. Error: %s\n", path, strerror(errno));
        return NULL;
    }

    /* Aquire a lock on the semaphore */
    if(sem_wait(sem) < 0) {
        syslog(LOG_ERR, "lpsampler_aquire_and_map failed to decrementsem %s. Error: %s\n", path, strerror(errno));
        return NULL;
    }

    /* Get the file descriptor for the shared memory segment */
    if((fd = shm_open(path, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpsampler_aquire_and_map Could not open shared memory segment. (%s) %s\n", path, strerror(errno));
        return NULL;
    }

    /* Get the size of the segment */
    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "lpsampler_aquire_and_map Could not stat shm. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* Attach the shared memory to the pointer */
    if((buf = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpsampler_aquire_and_map Could not mmap shared memory segment to size %ld. (%s) %s\n", statbuf.st_size, path, strerror(errno));
        return NULL;
    }

    /* Clean up sempahore resources */
    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpsampler_aquire_and_map sem_close Could not close semaphore\n");
        return NULL;
    }

    close(fd);

    return buf;
}


int lpsampler_aquire(char * name) {
    sem_t * sem;
    char path[PATH_MAX] = {0};
    lpsampler_get_path(name, path);

    /* Open the semaphore */
    if((sem = sem_open(path, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpsampler_aquire failed to open semaphore %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    /* Aquire a lock on the semaphore */
    if(sem_wait(sem) < 0) {
        syslog(LOG_ERR, "lpsampler_aquire failed to decrementsem %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    /* Clean up sempahore resources */
    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpsampler_aquire sem_close Could not close semaphore\n");
        return -1;
    }

    return 0;
}

int lpsampler_release(char * name) {
    sem_t * sem;
    char path[PATH_MAX] = {0};
    lpsampler_get_path(name, path);

    /* Open the semaphore */
    if((sem = sem_open(path, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpipc_buffer_release failed to open semaphore %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    /* Release the lock on the semaphore */
    if(sem_post(sem) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release failed to unlock %s. Error: %s\n", path, strerror(errno));
        return -1;
    }

    /* Clean up sempahore resources */
    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpipc_buffer_release sem_close Could not close semaphore\n");
        return -1;
    }

    return 0;
}

int lpsampler_destroy(char * name) {
    char path[PATH_MAX] = {0};
    lpsampler_get_path(name, path);

    /* Unlink the shared memory buffer */
    if(shm_unlink(path) < 0) {
        syslog(LOG_ERR, "lpsampler_destroy shm_unlink. Error: %s\n", strerror(errno));
        return -1;

    }

    /* Unlink the sempahore */
    if(sem_unlink(path) < 0) {
        syslog(LOG_ERR, "lpsampler_destroy sem_unlink Could not destroy semaphore\n");
        return -1;
    }

    return 0;
}

int lpsampler_write_ringbuffer_block(
        char * name, 
        lpbuffer_t * buf,
        float ** block, 
        int channels, 
        size_t blocksize_in_frames
    ) {
    size_t insert_pos, i, boundry;
    int c;
    float * channelp;
    float sample = 0;
    char path[PATH_MAX] = {0};

    lpsampler_get_path(name, path);

    /* Aquire a lock on the buffer */
    if(lpsampler_aquire(name) < 0) {
        syslog(LOG_ERR, "lpsampler_write_ringbuffer_block: Could not aquire ADC buffer shm for update\n");
        return -1;
    }

    assert(buf->channels == channels);

    boundry = buf->length * channels;

    /* Copy the block */
    for(c=0; c < channels; c++) {
        channelp = block[c];
        for(i=0; i < blocksize_in_frames; i++) {
            insert_pos = ((buf->pos+i) * channels + c) % boundry;
            sample = *channelp++;
            buf->data[insert_pos] = sample;
        }
    }

    /* Increment the write position */
    buf->pos += blocksize_in_frames;
    while(buf->pos >= buf->length) {
        buf->pos -= buf->length;
    }

    /* Release the lock on the ADC buffer shm */
    if(lpsampler_release(name) < 0) {
        syslog(LOG_ERR, "lpsampler_write_ringbuffer_block: Could not release buffer shm after update\n");
        return -1;
    }

    return 0;
}

int lpsampler_read_ringbuffer_block(
        char * name, 
        lpbuffer_t * buf,
        size_t offset_in_frames, 
        lpbuffer_t * out
    ) {
    size_t start, i, bufidx;
    int c;
    char path[PATH_MAX] = {0};

    lpsampler_get_path(name, path);

    syslog(LOG_INFO, "READ RINGBUF: aquiring lock on  %s\n", name);
    /* Aquire a lock on the buffer */
    if(lpsampler_aquire(name) < 0) {
        syslog(LOG_ERR, "Could not aquire ADC buffer shm for update\n");
        return -1;
    }

    syslog(LOG_INFO, "READ RINGBUF: adc buffer has value 10 %f\n", buf->data[10]);

    /*
     * buf->pos is the last frame written to the circular buffer
     * offset is the number of frames backward from that point to start reading
     * start is buf->pos - offset, wrapped to the length of the circular buffer
     */

    start = (buf->pos - offset_in_frames - out->length) % buf->length;
    for(i=0; i < out->length; i++) {
        bufidx = (start + i) % buf->length;
        for(c=0; c < buf->channels; c++) {
            out->data[i * buf->channels + c] = buf->data[bufidx * buf->channels + c];
        }
    }

    syslog(LOG_INFO, "COPY RINGBUF start=%ld buf->length=%ld out->length=%ld\n", 
            start, buf->length, out->length);

    /* Release the lock on the buffer shm */
    if(lpsampler_release(name) < 0) {
        syslog(LOG_ERR, "Could not release ADC buffer shm after update\n");
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
    char cc_path[50];
    size_t path_length;

    path_length = snprintf(NULL, 0, ASTRID_MIDI_CCBASE_PATH, device_id, cc) + 1;
    snprintf(cc_path, path_length,  ASTRID_MIDI_CCBASE_PATH, device_id, cc);

    return lpipc_getid(cc_path);
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

    path_length = snprintf(NULL, 0, ASTRID_MIDI_NOTEBASE_PATH, device_id, note) + 1;
    note_path = (char *)calloc(1, path_length);
    snprintf(note_path, path_length,  ASTRID_MIDI_NOTEBASE_PATH, device_id, note);

    return lpipc_getid(note_path);
}


/* SERIAL STATUS IPC
 * GETTERS & SETTERS
 * ****************/
int lpserial_setctl(int device_id, int param_id, size_t value) {
    char * ctl_path;
    size_t path_length;

    path_length = snprintf(NULL, 0, ASTRID_SERIAL_CTLBASE_PATH, device_id, param_id) + 1;
    ctl_path = (char *)calloc(1, path_length);
    snprintf(ctl_path, path_length,  ASTRID_SERIAL_CTLBASE_PATH, device_id, param_id);

    if(lpipc_setvalue(ctl_path, &value, sizeof(size_t)) < 0) {
        syslog(LOG_ERR, "Could not store %ld for MIDI CC %d from device %d\n", value, param_id, device_id);
        return -1;
    }

    free(ctl_path);

    return 0;
}

int lpserial_getctl(int device_id, int ctl, lpfloat_t * value) {
    char ctl_path[50];
    size_t path_length;
    size_t ctl_value = 0;
    void * ctl_valuep = &ctl_value;

    path_length = snprintf(NULL, 0, ASTRID_SERIAL_CTLBASE_PATH, device_id, ctl) + 1;
    snprintf(ctl_path, path_length,  ASTRID_SERIAL_CTLBASE_PATH, device_id, ctl);

    if(lpipc_getvalue(ctl_path, &ctl_valuep)) {
        return -1;        
    }

    *value = (lpfloat_t)ctl_value / (lpfloat_t)SIZE_MAX;

    return 0;
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

    if((fd = open(notemap_path, O_RDWR | O_CREAT | O_APPEND, LPIPC_PERMS)) < 0) {
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
        printf("msg.type: %d msg.initiated: %f msg.instrument_name: %s\n", msg.type, msg.initiated, msg.instrument_name);
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
    double now = 0;
    lpmsg_t msg = {0};

    notemap_path_length = snprintf(NULL, 0, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note) + 1;
    notemap_path = (char *)calloc(1, notemap_path_length);
    snprintf(notemap_path, notemap_path_length, ASTRID_MIDIMAP_NOTEBASE_PATH, device_id, note);

    if(access(notemap_path, F_OK) < 0) {
        syslog(LOG_DEBUG, "No notemap for %s\n", notemap_path);
        return 0;
    }

    if((fd = open(notemap_path, O_RDWR)) < 0) {
        syslog(LOG_ERR, "Could not open notemap file for triggering. Error: %s\n", strerror(errno));
        return -1;
    }

    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "Could not stat notemap file for triggering. Error: %s\n", strerror(errno));
        return -1;
    }

    notemap_size = statbuf.st_size;
    for(read_pos=0; read_pos < notemap_size; read_pos += sizeof(lpmsg_t)) {
        if((bytes_read = read(fd, &msg, sizeof(lpmsg_t))) < 0) {
            syslog(LOG_ERR, "Could not read msg from file during notemap trigger walk. Error: %s\n", strerror(errno));
            return -1;
        }

        if(msg.type == LPMSG_EMPTY) continue;

        if(lpscheduler_get_now_seconds(&now) < 0) {
            syslog(LOG_ERR, "Could not get now seconds during notemap trigger. Error: %s\n", strerror(errno));
            return -1;
        }

        msg.initiated = now;

        syslog(LOG_INFO, "Sending message from lpmidi trigger notemap\ninitiated %f\nscheduled %f\ncompleted %f\nmax_processing_time %f\nonset_delay %ld\nvoice_id %ld\ncount %ld\ntype %d\nmsg %s\nname %s\n\n", 
            msg.initiated,
            msg.scheduled,
            msg.completed,
            msg.max_processing_time,
            msg.onset_delay,
            msg.voice_id, 
            msg.count,
            msg.type,
            msg.msg,
            msg.instrument_name
        );

        if(send_message(msg.instrument_name, msg) < 0) {
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

int lpmidi_relay_to_instrument(char * instrument_name, unsigned char mtype, unsigned char mid, unsigned char mval) {
    lpmsg_t msg = {0};
    size_t offset = 0;

    init_instrument_message(&msg, instrument_name);
    msg.type = LPMSG_MIDI_FROM_DEVICE;

    memcpy(msg.msg, &mtype, sizeof(unsigned char));
    offset += sizeof(unsigned char);

    memcpy(msg.msg+offset, &mid, sizeof(unsigned char));
    offset += sizeof(unsigned char);

    memcpy(msg.msg+offset, &mval, sizeof(unsigned char));

    send_play_message(msg);

    return 0;
}

/* VOICES
 * ******/
#if 0
ssize_t astrid_get_voice_id() {
    size_t voice_id = 0;
    void * voice_idp = &voice_id;

    if(access("/tmp/astridvoiceid", F_OK) != 0) {
        if(lpipc_createvalue("/tmp/astridvoiceid", sizeof(size_t)) < 0) {
            syslog(LOG_ERR, "astrid_get_voice_id failed to create value. Error: %s\n", strerror(errno));
            return -1;
        }

        if(lpipc_setid("/tmp/astridvoiceid", 0) < 0) {
            syslog(LOG_ERR, "astrid_get_voice_id failed to set id for value. Error: %s\n", strerror(errno));
            return -1;
        }
        
        if(lpipc_unsafe_getvalue("/tmp/astridvoiceid", &voice_idp) < 0) {
            syslog(LOG_ERR, "astrid_get_voice_id failed to get value. Error: %s\n", strerror(errno));
            return -1;
        }

    } else {
        if(lpipc_getvalue("/tmp/astridvoiceid", &voice_idp) < 0) {
            syslog(LOG_ERR, "astrid_get_voice_id failed to get value. Error: %s\n", strerror(errno));
            return -1;
        }
    }

    if(lpipc_releasevalue("/tmp/astridvoiceid") < 0) {
        syslog(LOG_ERR, "astrid_get_voice_id failed to release lock on value after read. Error: %s\n", strerror(errno));
        return -1;
    }

    voice_id += 1;

    if(lpipc_setvalue("/tmp/astridvoiceid", voice_idp, sizeof(size_t)) < 0) {
        syslog(LOG_ERR, "astrid_get_voice_id failed to release lock on value after read. Error: %s\n", strerror(errno));
        return -1;
    }

    return voice_id;
}
#endif
int lpcounter_get_path(char * name, char * path) {
    snprintf(path, PATH_MAX, "/astrid-counter-%s", name);
    return 0;
}

ssize_t lpcounter_create(char * name) {
    sem_t * sem;
    void * shmaddr;
    int shmfd;
    size_t counter_val = 0;
    char path[PATH_MAX] = {0};

    lpsampler_get_path(name, path);
    syslog(LOG_DEBUG, "creating %s counter at path %s\n", name, path);

    // create the semaphore and aquire a lock on it
    if((sem = sem_open(path, O_CREAT, LPIPC_PERMS, 1)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpcounter_create failed to create semaphore %s. Error: %s\n", name, strerror(errno));
        return -1;
    }

    // create shared memory segment
    if((shmfd = shm_open(path, O_CREAT | O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpcounter_create Could not create shared memory segment. (%s) %s\n", name, strerror(errno));
        return -1;
    }

    // zero it
    if(ftruncate(shmfd, sizeof(size_t)) < 0) {
        syslog(LOG_ERR, "lpcounter_create Could not truncate shared memory segment to size %ld. (%s) %s\n", sizeof(size_t), name, strerror(errno));
        return -1;
    }
   
    // Attach to the shared memory
    if((shmaddr = mmap(NULL, sizeof(size_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpcounter_create Could not mmap shared memory segment. (%s) %s\n", name, strerror(errno));
        return -1;
    }

    // Initialize the shared memory with 0
    memcpy(shmaddr, &counter_val, sizeof(size_t)); 

    munmap(shmaddr, sizeof(size_t));
    close(shmfd);

    return counter_val;
}

int lpcounter_destroy(char * name) {
    char path[PATH_MAX] = {0};
    lpcounter_get_path(name, path);

    /* Unlink the shared memory buffer */
    if(shm_unlink(path) < 0) {
        syslog(LOG_ERR, "lpcounter_destroy shm_unlink. Error: %s\n", strerror(errno));
        return -1;

    }

    /* Unlink the sempahore */
    if(sem_unlink(path) < 0) {
        syslog(LOG_ERR, "lpcounter_destroy sem_unlink Could not destroy semaphore\n");
        return -1;
    }

    return 0;
}

ssize_t lpcounter_read_and_increment(char * name) {
    size_t counter_val = 0;
    size_t counter_val_next = 0;
    void * counter_valp = &counter_val;
    void * counter_val_nextp = &counter_val_next;
    int shmfd;
    sem_t * sem;
    void * shmaddr;
    char path[PATH_MAX] = {0};

    lpsampler_get_path(name, path);
    syslog(LOG_DEBUG, "creating %s counter at path %s\n", name, path);

    // Open the semaphore
    if((sem = sem_open(path, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "lpipc_getvalue failed to open semaphore %s. Error: %s\n", name, strerror(errno));
        return -1;
    }

    // Aquire a lock on the semaphore
    if(sem_wait(sem) < 0) {
        syslog(LOG_ERR, "lpipc_getvalue failed to decrement sem %s. Error: %s\n", name, strerror(errno));
        return -1;
    }

    // Attach to the shared memory
    if((shmfd = shm_open(path, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "lpipc_getvalue Could not open shared memory segment. (%s) %s\n", name, strerror(errno));
        return -1;
    }

    if((shmaddr = mmap(NULL, sizeof(size_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_getvalue Could not mmap shared memory segment. (%s) %s\n", name, strerror(errno));
        return -1;
    }

    // Copy the voice id
    memcpy(counter_valp, shmaddr, sizeof(size_t));

    // increment the voice id and copy it back to the shm
    counter_val_next = counter_val + 1;
    memcpy(shmaddr, counter_val_nextp, sizeof(size_t));

    // Release the lock on the semaphore
    if(sem_post(sem) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue failed to unlock %s. Error: %s\n", name, strerror(errno));
        return -1;
    }

    if(sem_close(sem) < 0) {
        syslog(LOG_ERR, "lpipc_setvalue sem_close Could not close semaphore\n");
        return -1;
    }

    munmap(shmaddr, sizeof(size_t));

    close(shmfd);

    return counter_val;
}


/* BUFFER
 * SERIALIZATION
 * *************/
unsigned char * serialize_buffer(lpbuffer_t * buf, lpmsg_t * msg, size_t * strsize) {
    size_t audiosize, offset;
    unsigned char * str;

    audiosize = buf->length * buf->channels * sizeof(lpfloat_t);

    *strsize =  0;
    *strsize += sizeof(size_t);  /* audio size in bytes */
    *strsize += sizeof(size_t);  /* audio length in frames */
    *strsize += sizeof(int);     /* channels   */
    *strsize += sizeof(int);     /* samplerate */
    *strsize += sizeof(int);     /* is_looping */
    *strsize += sizeof(size_t);  /* onset      */
    *strsize += audiosize;       /* audio data */
    *strsize += sizeof(lpmsg_t); /* message */

    /* initialize string buffer */
    str = (unsigned char *)calloc(1, *strsize);

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

lpbuffer_t * deserialize_buffer(char * buffer_code, lpmsg_t * msg) {
    size_t audiosize, offset, length, onset;
    int channels, samplerate, is_looping;
    unsigned char * str; // bufstr
    lpbuffer_t * buf;
    struct stat statbuf;
    int fd;
    sem_t * sem;
    void * shmaddr;

    /* Open the semaphore */
    if((sem = sem_open(buffer_code, 0, LPIPC_PERMS)) == SEM_FAILED) {
        syslog(LOG_ERR, "deserialize_buffer: failed to open semaphore %s. Error: %s\n", buffer_code, strerror(errno));
        return NULL;
    }

    /* Aquire a lock on the semaphore */
    if(sem_wait(sem) < 0) {
        syslog(LOG_ERR, "deserialize_buffer: failed to decrementsem %s. Error: %s\n", buffer_code, strerror(errno));
        return NULL;
    }

    /* Get the file descriptor for the shared memory segment */
    if((fd = shm_open(buffer_code, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "deserialize_buffer: Could not open shared memory segment. (%s) %s\n", buffer_code, strerror(errno));
        return NULL;
    }

    /* Get the size of the segment */
    if(fstat(fd, &statbuf) < 0) {
        syslog(LOG_ERR, "deserialize_buffer: Could not stat shm. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* Attach the shared memory to the pointer */
    if((shmaddr = (void*)mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "deserialize_buffer: Could not mmap shared memory segment to size %ld. (%s) %s\n", statbuf.st_size, buffer_code, strerror(errno));
        return NULL;
    }

    str = (unsigned char *)LPMemoryPool.alloc(1, statbuf.st_size);
    memcpy(str, shmaddr, statbuf.st_size);

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

    buf = (lpbuffer_t *)LPMemoryPool.alloc(1, sizeof(lpbuffer_t) + audiosize);
    memcpy(buf->data, str + offset, audiosize);
    offset += audiosize;

    memcpy(msg, str + offset, sizeof(lpmsg_t));
    offset += sizeof(lpmsg_t);

    buf->length = length;
    buf->channels = channels;
    buf->samplerate = samplerate;
    buf->is_looping = is_looping;
    buf->onset = onset;

    buf->phase = 0.f;
    buf->pos = 0;
    buf->boundry = length-1;
    buf->range = length;

    /* cleanup the bufstr shared memory*/
    LPMemoryPool.free(str);

    /* unmap the shared memory */
    if(munmap(shmaddr, statbuf.st_size) < 0) {
        syslog(LOG_ERR, "deserialize_buffer munmap. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* Unlink the shared memory buffer */
    if(shm_unlink(buffer_code) < 0) {
        syslog(LOG_ERR, "deserialize_buffer shm_unlink. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* Unlink the sempahore */
    if(sem_unlink(buffer_code) < 0) {
        syslog(LOG_ERR, "deserialize_buffer sem_unlink Could not destroy semaphore\n");
        return NULL;
    }

    close(fd);

    return buf;
}

/* MESSAGE
 * QUEUES
 * ******/

int init_instrument_message(lpmsg_t * msg, char * instrument_name) {
    memset(msg, 0, sizeof(lpmsg_t));
    strncpy(msg->instrument_name, instrument_name, LPMAXNAME-1);
    return 0; 
}

lpmsg_t * create_instrument_message(char * instrument_name) {
    lpmsg_t * msg;
    msg = LPMemoryPool.alloc(1, sizeof(lpmsg_t));
    if(init_instrument_message(msg, instrument_name) < 0) {
        return NULL;
    }

    return msg;
}

int send_play_message(lpmsg_t msg) {
    mqd_t mqd;
    char qname[NAME_MAX] = {0};
    struct mq_attr attr;

    attr.mq_maxmsg = ASTRID_MQ_MAXMSG;
    attr.mq_msgsize = sizeof(lpmsg_t);

    snprintf(qname, NAME_MAX, "/%s-msgq", msg.instrument_name);

    if((mqd = mq_open(qname, O_CREAT | O_WRONLY, LPIPC_PERMS, &attr)) == (mqd_t) -1) {
        syslog(LOG_ERR, "send_play_message mq_open: Error opening message queue. Error: %s\n", strerror(errno));
        return -1;
    }

    if(mq_send(mqd, (char *)(&msg), sizeof(lpmsg_t), 0) < 0) {
        syslog(LOG_ERR, "send_play_message mq_send: Error allocing during message write. Error: %s\n", strerror(errno));
        return -1;
    }

    if(mq_close(mqd) == -1) {
        syslog(LOG_ERR, "send_play_message close: Error closing play queue. Error: %s\n", strerror(errno));
        return -1; 
    }

    return 0;
}

int send_serial_message(lpmsg_t msg) {
    /* Relays a message to the serial listener thread for a 
     * given instrument, which will write the translated payload
     * to the tty... only used for special solenoid trigger & 
     * motor speed messages now... also for shutdown messages
     * FIXME ... & probably needs some decode/encode fixes after recent changes
     */
    mqd_t mqd;
    char qname[NAME_MAX] = {0};
    struct mq_attr attr;

    attr.mq_maxmsg = ASTRID_MQ_MAXMSG;
    attr.mq_msgsize = sizeof(lpmsg_t);

    snprintf(qname, NAME_MAX, "/%s-serial-msgq", msg.instrument_name);

    if((mqd = mq_open(qname, O_CREAT | O_WRONLY, LPIPC_PERMS, &attr)) == (mqd_t) -1) {
        syslog(LOG_ERR, "send_serial_message mq_open: Error opening message queue. Error: %s\n", strerror(errno));
        return -1;
    }

    if(mq_send(mqd, (char *)(&msg), sizeof(lpmsg_t), 0) < 0) {
        syslog(LOG_ERR, "send_serial_message mq_send: Error allocing during message write. Error: %s\n", strerror(errno));
        return -1;
    }

    if(mq_close(mqd) == -1) {
        syslog(LOG_ERR, "send_serial_message close: Error closing play queue. Error: %s\n", strerror(errno));
        return -1; 
    }

    return 0;
}


int send_message(char * qname, lpmsg_t msg) {
    mqd_t mqd;
    struct mq_attr attr;

    attr.mq_maxmsg = ASTRID_MQ_MAXMSG;
    attr.mq_msgsize = sizeof(lpmsg_t);

    syslog(LOG_DEBUG, "Sending message type %d on queue %s\n", msg.type, qname);

    if((mqd = mq_open(qname, O_CREAT | O_WRONLY, LPIPC_PERMS, &attr)) == (mqd_t) -1) {
        syslog(LOG_ERR, "send_message mq_open: Error opening message queue. Error: %s\n", strerror(errno));
        return -1;
    }

    if(mq_send(mqd, (char *)(&msg), sizeof(lpmsg_t), 0) < 0) {
        syslog(LOG_ERR, "send_message mq_send: Error allocing during message write. Error: %s\n", strerror(errno));
        return -1;
    }

    if(mq_close(mqd) == -1) {
        syslog(LOG_ERR, "send_message close: Error closing message relay queue. Error: %s\n", strerror(errno));
        return -1; 
    }

    /*syslog(LOG_DEBUG, "Finished sending message type %d to %s\n", msg.type, msg.instrument_name);*/

    return 0;
}

mqd_t astrid_playq_open(const char * instrument_name) {
    mqd_t mqd;
    ssize_t qname_length;
    char qname[LPMAXQNAME] = {0};
    struct mq_attr attr;

    attr.mq_maxmsg = ASTRID_MQ_MAXMSG;
    attr.mq_msgsize = sizeof(lpmsg_t);

    qname_length = snprintf(NULL, 0, "%s-%s", LPPLAYQ, instrument_name) + 1;
    qname_length = (LPMAXQNAME >= qname_length) ? LPMAXQNAME : qname_length;
    snprintf(qname, qname_length, "%s-%s", LPPLAYQ, instrument_name);

    syslog(LOG_DEBUG, "Opening playq %s\n", qname);
    if((mqd = mq_open(qname, O_CREAT | O_RDONLY, LPIPC_PERMS, &attr)) == (mqd_t) -1) {
        syslog(LOG_ERR, "astrid_playq_open mq_open: Error opening message queue. Error: %s\n", strerror(errno));
        return -1;
    }

    syslog(LOG_DEBUG, "Opened playq mqd:%d\n", mqd);
    return mqd;
}

int astrid_playq_close(mqd_t mqd) {
    if(mq_close(mqd) == -1) {
        syslog(LOG_ERR, "astrid_playq_close close: Error closing play queue FIFO. Error: %s\n", strerror(errno));
        return -1; 
    }

    return 0;
}

int astrid_playq_read(mqd_t mqd, lpmsg_t * msg) {
    char * msgp;
    ssize_t read_result;
    unsigned int msg_priority;

    msgp = (char *)msg;

    if((read_result = mq_receive(mqd, msgp, sizeof(lpmsg_t), &msg_priority)) < 0) {
        syslog(LOG_ERR, "astrid_playq_read mq_receive: Error allocing during message read. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

mqd_t astrid_msgq_open(char * qname) {
    mqd_t mqd;
    struct mq_attr attr;

    attr.mq_maxmsg = ASTRID_MQ_MAXMSG;
    attr.mq_msgsize = sizeof(lpmsg_t);

    syslog(LOG_DEBUG, "Opening msgq %s\n", qname);
    if((mqd = mq_open(qname, O_CREAT | O_RDONLY, LPIPC_PERMS, &attr)) == (mqd_t) -1) {
        syslog(LOG_ERR, "astrid_playq_open mq_open: Error opening message queue. Error: %s\n", strerror(errno));
        return (mqd_t) -1;
    }

    syslog(LOG_DEBUG, "Opened msgq mqd:%d\n", mqd);
    return mqd;
}

int astrid_msgq_close(mqd_t mqd) {
    if(mq_close(mqd) == -1) {
        syslog(LOG_ERR, "astrid_msgq_close close: Error closing msg queue FIFO. Error: %s\n", strerror(errno));
        return -1; 
    }

    return 0;
}

int astrid_msgq_read(mqd_t mqd, lpmsg_t * msg) {
    char * msgp;
    ssize_t read_result;
    unsigned int msg_priority = 0;

    syslog(LOG_DEBUG, "Reading from msgq mqd:%d\n", mqd);
    syslog(LOG_DEBUG, "Reading from msg.instrument_name:%s\n", msg->instrument_name);
    msgp = (char *)msg;

    if((read_result = mq_receive(mqd, msgp, sizeof(lpmsg_t), &msg_priority)) < 0) {
        syslog(LOG_ERR, "astrid_msgq_read mq_receive: Error reading message. (Got %ld bytes) Error: %s\n", read_result, strerror(errno));
        return -1;
    }

    syslog(LOG_DEBUG, "msg.msg is now:%s\n", msg->msg);

    return 0;
}

int astrid_get_playback_device_id() {
    int device_id;

    /* If the default device isn't set, then 
     * set it to zero and return the ID */
    if(access(ASTRID_DEVICEID_PATH, F_OK) < 0) {
        if(lpipc_setid(ASTRID_DEVICEID_PATH, 0) < 0) {
            syslog(LOG_ERR, "Could not set default device ID to 0. Error: (%d) %s\n", errno, strerror(errno));
            return -1;
        }

        return 0;
    }

    /* Otherwise look up the ID */
    if((device_id = lpipc_getid(ASTRID_DEVICEID_PATH)) < 0) {
        syslog(LOG_ERR, "Could not get current device ID. Error: (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    return device_id;
}

/* TODO set these per-process and per-device */
int astrid_get_capture_device_id() {
    return astrid_get_playback_device_id();
}

int extract_int32_from_token(char * token, int32_t * val) {
    char * end;
    long result = 0;

    errno = 0;
    result = strtol(token, &end, 0);

    if(errno != 0) return -1;
    if(*end != '\0') return -1;

    if(result > INT_MAX) result = INT_MAX;

    *val = (int32_t)result;
    return 0;
}

int extract_float_from_token(char * token, float * val) {
    char * end;
    float result = 0;

    errno = 0;
    result = strtof(token, &end);

    if(errno != 0) return -1;
    if(*end != '\0') return -1;

    *val = result;
    return 0;
}

int extract_floatlist_from_token(char * tokenlist, lpfloat_t * val, int size) {
    char * end, * save, * token;
    char tokenlist_copy[LPMAXMSG] = {0};
    float result = 0;
    int count = 0, i = 0;

    memcpy(tokenlist_copy, tokenlist, LPMAXMSG);
    token = strtok_r(tokenlist_copy, ",", &save);

    while(token != NULL) {
        errno = 0;
        result = strtof(token, &end);

        if(errno != 0) return -1;
        if(*end != '\0') return -1;
        val[count] = result;

        count += 1;
        if(count >= size) break;

        token = strtok_r(NULL, ",", &save);
    }

    for(i=count; i < size; i++) {
        val[i] = val[0];
    }

    return count;
}


int extract_patternbuf_from_token(char * token, unsigned char * patternbuf, size_t * pattern_length) {
    size_t i;

    *pattern_length = strlen(token);
    if(*pattern_length > LPMAXPAT) *pattern_length = LPMAXPAT;

    memset(patternbuf, 0, LPMAXPAT);
    for(i=0; i < *pattern_length; i++) {
        patternbuf[i] = (token[i] == '1');
    }

    return 0;
}

int decode_update_message_param(lpmsg_t * msg, uint16_t * id, float * value) {
    memcpy(id, msg->msg, sizeof(uint16_t));
    memcpy(value, msg->msg + sizeof(uint16_t), sizeof(float));
    return 0;
}

/* FIXME these encode* functions can probably go away...? */
int encode_update_message_param(lpmsg_t * msg) {
    char *token, *save=NULL;
    char param_copy[LPMAXMSG] = {0};

    int32_t vali32 = 0;
    int16_t vali16 = 0;
    float valf = 0.f;

    memcpy(param_copy, msg->msg, LPMAXMSG);
    token = strtok_r(param_copy, " ", &save);

    if(extract_int32_from_token(token, &vali32) < 0) {
        return -1;
    } 

    syslog(LOG_DEBUG, "ENCODE UPDATE got ID %d\n", vali32);
    if(vali32 > 32767) vali32 = 32767;
    vali16 = (uint16_t)vali32;
    memcpy(msg->msg, &vali16, sizeof(uint16_t));

    if(extract_float_from_token(token, &valf) == 0) {
        return -1;
    }

    syslog(LOG_DEBUG, "ENCODE UPDATE got VALUE %f\n", valf);
    memcpy(msg->msg + sizeof(uint16_t), &valf, sizeof(float));
   
    return 0;
}

int encode_update_message_param_float(uint16_t id, char * valstr, lpmsg_t * msg) {
    float val = 0;
    if(extract_float_from_token(valstr, &val) < 0) {
        syslog(LOG_ERR, "encode_update_message_param_float: Could not extract float from token '%s'\n", valstr);
        return -1;
    }

    memset(msg->msg, 0, sizeof(LPMAXMSG));
    memcpy(msg->msg, &id, sizeof(uint16_t));
    memcpy(msg->msg + sizeof(uint16_t), &val, sizeof(float));

    return 0;
}

int encode_update_message_param_int32(uint16_t id, char * valstr, lpmsg_t * msg) {
    int32_t val = 0;
    int16_t val16 = 0;
    if(extract_int32_from_token(valstr, &val) < 0) {
        syslog(LOG_ERR, "encode_update_message_param_int32: Could not extract int32_t from token '%s'\n", valstr);
        return -1;
    }

    if(val > 32767) val = 32767;
    val16 = (uint16_t)val;
    memset(msg->msg, 0, sizeof(LPMAXMSG));
    memcpy(msg->msg, &id, sizeof(uint16_t));
    memcpy(msg->msg + sizeof(uint16_t), &val16, sizeof(uint16_t));

    return 0;
}

// Takes the cmd on the instrument and runs each param through the 
// instrument's param update callback.
int process_param_updates(lpinstrument_t * instrument) {
    char cmdline[LPMAXMSG] = {0};
    char * paramline;
    char * keytoken;
    char * valtoken;
    char * cmdline_save;
    char * paramline_save;

    memcpy(cmdline, instrument->msg.msg, LPMAXMSG);

    paramline = strtok_r(cmdline, " ", &cmdline_save);
    while(paramline != NULL) {
        keytoken = strtok_r(paramline, "=", &paramline_save);
        if(keytoken != NULL) {
            valtoken = strtok_r(NULL, "=", &paramline_save);
            if(valtoken != NULL) {
                syslog(LOG_DEBUG, "UPDATE Key: %s, Value: %s\n", keytoken, valtoken);
                // TODO think about handling large / special values, like buffers...
                // payload could be the shm ID like render complete messages...?
                if(instrument->update(instrument, keytoken, valtoken) < 0) {
                    syslog(LOG_ERR, "process_param_updates: failed to pass param (%s=%s) to update callback.\n", keytoken, valtoken);
                }
            }
        }
        paramline = strtok_r(NULL, " ", &cmdline_save);
    }
    return 0;
}


int prepare_and_send_instrument_message(lpmsg_t * msg, char msgtype) {
    int voice_id = 0;

    /* Get the voice ID from the voice ID counter */
    if((voice_id = lpcounter_read_and_increment("voiceid")) < 0) {
        syslog(LOG_ERR, "Error getting voice ID: (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    /* Initialize the count and set the voice_id */
    msg->count = 0;
    msg->voice_id = voice_id;

    switch(msgtype) {
        case PLAY_MESSAGE:
            msg->type = LPMSG_PLAY;
            break;

        case TRIGGER_MESSAGE:
            msg->type = LPMSG_TRIGGER;
            break;

        case SERIAL_MESSAGE:
            msg->type = LPMSG_SERIAL;
            break;

        case UPDATE_MESSAGE:
            msg->type = LPMSG_UPDATE;
            break;

        case LOAD_MESSAGE:
            msg->type = LPMSG_LOAD;
            break;

        case SCHEDULE_MESSAGE:
            msg->type = LPMSG_SCHEDULE;
            break;

        case SHUTDOWN_MESSAGE:
            msg->type = LPMSG_SHUTDOWN;
            break;

        case SET_COUNTER_MESSAGE:
            msg->type = LPMSG_SET_COUNTER;
            break;

        default:
            syslog(LOG_CRIT, "Bad msgtype! %c\n", msgtype);
            return -1;
    }

    if(send_play_message(*msg) < 0) {
        syslog(LOG_ERR, "Could not send play message...\n");
        return -1;
    }

    // FIXME -- maybe set this elsewhere to let cleanup happen first?
    //if(msg->type == LPMSG_SHUTDOWN) instrument->is_running = 0;

    return 0;
}

int parse_message_from_args(int argc, int arg_offset, char * argv[], lpmsg_t * msg) {
    int bytesread, a, i, length;
    char msgtype;
    char message_params[LPMAXMSG] = {0};
    char instrument_name[LPMAXNAME] = {0};
    size_t instrument_name_length;

    instrument_name_length = 0;
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

    if(prepare_and_send_instrument_message(msg, msgtype) < 0) {
        syslog(LOG_ERR, "Could not prepare message of type %c: (%d) %s\n", msgtype, errno, strerror(errno));
        return -1;
    }

    return 0;
}

int parse_message_from_cmdline(char * cmdline, size_t cmdlength, lpmsg_t * msg) {
    int tokenlength;
    char *token, *save=NULL;
    char msgtype;
    char cmd_copy[LPMAXMSG] = {0};

    if(cmdline == NULL) return 0;

    // strtok clobbers input
    memcpy(cmd_copy, cmdline, cmdlength);

    // Get the message type, and ignore everything else for now...
    token = strtok_r(cmd_copy, " ", &save);
    msgtype = token[0]; // msg types only use the first char
    tokenlength = strlen(token);

    memset(msg->msg, 0, LPMAXMSG);
    strncpy(msg->msg, cmdline + tokenlength, cmdlength - tokenlength);

    if(prepare_and_send_instrument_message(msg, msgtype) < 0) {
        syslog(LOG_ERR, "Could not prepare message of type %c: (%d) %s\n", msgtype, errno, strerror(errno));
        return -1;
    }

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
            syslog(LOG_DEBUG, "    e%ld onset: %ld pos: %ld length: %ld\n", current->id, current->onset, current->pos, current->buf->length);
            current = (lpevent_t *)current->next;        
        }
        syslog(LOG_DEBUG, "    e%ld onset: %ld pos: %ld length: %ld\n", current->id, current->onset, current->pos, current->buf->length);
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

    syslog(LOG_INFO, "START playing event ID %ld\n", e->id);

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

    syslog(LOG_INFO, "STOP playing event ID %ld\n", e->id);

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
    if(pthread_mutex_lock(&astrid_nursery_lock) != 0) {
        syslog(LOG_ERR, "Error getting lock on nursery mutex\n");
        return;
    }
    if(s->nursery_head == NULL) {
        s->nursery_head = e;
    } else {
        current = s->nursery_head;
        while(current->next != NULL) {
            current = (lpevent_t *)current->next;
        }
        current->next = (void *)e;
    }
    if(pthread_mutex_unlock(&astrid_nursery_lock) != 0) {
        syslog(LOG_ERR, "Error getting lock on nursery mutex\n");
        return;
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
            if(current->buf != NULL && current->pos >= current->buf->length-1) {
                stop_playing(s, current);
            }
            current = (lpevent_t *)next;
        }

        if(current->buf != NULL && current->pos >= current->buf->length-1) {
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
            if(current->buf != NULL && current->pos < current->buf->length) {
                bufc = c % current->buf->channels;
                sample += current->buf->data[current->pos * current->buf->channels + bufc];
            }
            current = (lpevent_t *)current->next;
        }

        if(current->buf != NULL && current->pos < current->buf->length) {
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
        syslog(LOG_DEBUG, "%d waiting\n", ll_count(s->waiting_queue_head));
        ll_display(s->waiting_queue_head);
    } else {
        syslog(LOG_DEBUG, "none waiting\n");
    }

    if(s->playing_stack_head) {
        syslog(LOG_DEBUG, "%d playing\n", ll_count(s->playing_stack_head));
        ll_display(s->playing_stack_head);
    } else {
        syslog(LOG_DEBUG, "none playing\n");
    }

    if(s->nursery_head) {
        syslog(LOG_DEBUG, "%d done\n\n", ll_count(s->nursery_head));
        ll_display(s->nursery_head);
    } else {
        syslog(LOG_DEBUG, "none done\n");
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

void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t onset_delay) {
    lpevent_t * e;

    /*
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
    */
    e = (lpevent_t *)LPMemoryPool.alloc(1, sizeof(lpevent_t));
    s->event_count += 1;
    e->id = s->event_count;
    e->onset = 0;
    e->callback_onset = 0;

    e->buf = buf;
    e->pos = 0;
    e->onset = s->ticks + onset_delay;

    syslog(LOG_INFO, "scheduling event ID %ld\n", e->id);
    syslog(LOG_INFO, "scheduler got buffer with onset %ld\n", e->onset);
    syslog(LOG_INFO, "scheduler got buffer value 10 %f\n", buf->data[10]);

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

    if(s->waiting_queue_head != NULL) {
        current = s->waiting_queue_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            LPMemoryPool.free(current);
            current = (lpevent_t *)next;        
        }
        LPMemoryPool.free(current);
    }

    if(s->playing_stack_head != NULL) {
        current = s->playing_stack_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            LPMemoryPool.free(current);
            current = (lpevent_t *)next;        
        }
        LPMemoryPool.free(current);
    }

    if(s->nursery_head != NULL) {
        current = s->nursery_head;
        while(current->next != NULL) {
            next = (lpevent_t *)current->next;
            LPMemoryPool.free(current);
            current = (lpevent_t *)next;        
        }
        LPMemoryPool.free(current);
    }
    LPMemoryPool.free(s->now);
    LPMemoryPool.free(s->current_frame);
    LPMemoryPool.free(s);
}

int scheduler_cleanup_nursery(lpscheduler_t * s) {
    /* Loop over nursery and free buffers */
    lpevent_t * current;
    lpevent_t * next;

    if(s->nursery_head == NULL) return 0;

    if(pthread_mutex_lock(&astrid_nursery_lock) != 0) {
        syslog(LOG_ERR, "Error getting lock on nursery mutex\n");
        return -1;
    }

    current = s->nursery_head;
    while(current->next != NULL) {
        next = (lpevent_t *)current->next;
        syslog(LOG_INFO, "freeing event ID %ld\n", current->id);
        LPBuffer.destroy(current->buf);
        free(current);
        current = next;        
    }

    if(current != NULL) {
        syslog(LOG_INFO, "freeing event ID %ld\n", current->id);
        LPBuffer.destroy(current->buf);
        free(current);
    }

    s->nursery_head = NULL;

    if(pthread_mutex_unlock(&astrid_nursery_lock) != 0) {
        syslog(LOG_ERR, "Error releasing lock on nursery mutex\n");
        return -1;
    }

    //scheduler_debug(s);
    return 0;
}

int astrid_instrument_jack_callback(jack_nframes_t nframes, void * arg) {
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    float * output_channels[instrument->channels];
    float * input_channels[instrument->channels];
    size_t i;
    int c;

    if(!instrument->is_running) return 0;

    if(!instrument->has_been_initialized) {
        syslog(LOG_DEBUG, "Seeding the random number generator from the audio callback. %s\n", instrument->name);
        LPRand.preseed();
        instrument->has_been_initialized = 1;
        // TODO could run the before callback here maybe, 
        // or is there a reason to have mainthread-before AND audiothread-before?
    }

    for(c=0; c < instrument->channels; c++) {
        input_channels[c] = (float *)jack_port_get_buffer(instrument->inports[c], nframes);
        output_channels[c] = (float *)jack_port_get_buffer(instrument->outports[c], nframes);
        memset(output_channels[c], 0, nframes * sizeof(float));
    }

    /* write the block into the adc ringbuffer */
    if(lpsampler_write_ringbuffer_block(instrument->adcname, instrument->adcbuf, input_channels, instrument->channels, nframes) < 0) {
        syslog(LOG_ERR, "Error writing into adc ringbuf\n");
        return 0;
    }

    /* mix in async renders */
    for(i=0; i < (size_t)nframes; i++) {
        lpscheduler_tick(instrument->async_mixer);
        for(c=0; c < instrument->channels; c++) {
            output_channels[c][i] += instrument->async_mixer->current_frame[c];
        }
    }

    if(instrument->stream != NULL) {
        if(instrument->stream((size_t)nframes, input_channels, output_channels, (void *)instrument) < 0) {
            return -1;
        }
    }

#if 0
    /* clamp output */
    for(c=0; c < instrument->channels; c++) {
        for(i=0; i < (size_t)nframes; i++) {
            output_channels[c][i] = fmax(-1.f, fmin(output_channels[c][i], 1.f));
        }
    }
#endif

    return 0;
}

/* instrument seq priority queue callbacks */
static int msgpq_cmp_pri(double next, double curr) {
    return (next > curr);
}

static double msgpq_get_pri(void * a) {
    return ((lpmsgpq_node_t *)a)->timestamp;
}

static void msgpq_set_pri(void * a, double timestamp) {
    ((lpmsgpq_node_t *)a)->timestamp = timestamp;
}

static size_t msgpq_get_pos(void * a) {
    return ((lpmsgpq_node_t *)a)->pos;
}

static void msgpq_set_pos(void * a, size_t pos) {
    ((lpmsgpq_node_t *)a)->pos = pos;
}

int instrument_seq_remove_nodes_by_voice_id(pqueue_t * msgpq, size_t voice_id) {
    lpmsgpq_node_t * node;
    lpmsg_t * msg;
    void * d;
    size_t i;

    for(i=0; i < msgpq->size; i++) {
        d = msgpq->d[i];                
        if(d == NULL) {
            syslog(LOG_DEBUG, "STOP: d[%d] is NULL\n", (int)i);
            continue;
        }

        node = (lpmsgpq_node_t *)d;
        msg = &node->msg;
        if(msg->voice_id == voice_id) {
            syslog(LOG_DEBUG, "STOP: removing node & freeing memory for voice id %ld\n", voice_id);
            pqueue_remove(msgpq, d);
        }
    }

    return 0;
}

void * instrument_seq_pq(void * arg) {
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    lpmsg_t * msg;
    lpmsgpq_node_t * node;
    void * d;
    double now;

    now = 0;
    syslog(LOG_DEBUG, ":::: INSTRUMENT SEQ PQ STARTING ::::\n");

    d = NULL;
    msg = NULL;
    node = NULL;

    while(instrument->is_running) {
        /* peek into the queue */
        d = pqueue_peek(instrument->msgpq);

        /* No messages have arrived */
        if(d == NULL) {
            usleep((useconds_t)500);
            continue;
        }

        /* There is a message! */
        node = (lpmsgpq_node_t *)d;
        msg = &node->msg;

        if(msg->type == LPMSG_SHUTDOWN) {
            break;
        }

        /* Get now */
        if(lpscheduler_get_now_seconds(&now) < 0) {
            syslog(LOG_CRIT, "Error getting now in message scheduler\n");
            exit(1);
        }

        /* If msg timestamp is in the future, 
         * sleep for a bit and then try again */
        if(node->timestamp > now) {
            usleep((useconds_t)500);
            continue;
        }

        /* Send it along to the instrument message fifo */
        if(send_play_message(*msg) < 0) {
            syslog(LOG_ERR, "Error sending play message from message priority queue\n");
            usleep((useconds_t)500);
            continue;
        }

        /* And remove it from the pq */
        if(pqueue_remove(instrument->msgpq, d) < 0) {
            syslog(LOG_ERR, "pqueue_remove: problem removing message from the pq\n");
            usleep((useconds_t)500);
        }
    }

    syslog(LOG_INFO, "Message scheduler pq thread shutting down...\n");
    return 0;
}

int relay_message_to_seq(lpinstrument_t * instrument) {
    lpmsgpq_node_t * d;
    double seq_delay, now=0;

    syslog(LOG_DEBUG, "SEQ PQ MSG: got a scheduled message to insert into the sequencer priority queue\n");
    syslog(LOG_DEBUG, "SEQ PQ MSG: pqnode_index=%d\n", instrument->pqnode_index);

    d = &instrument->pqnodes[instrument->pqnode_index];
    memcpy(&d->msg, &instrument->msg, sizeof(lpmsg_t));
    instrument->pqnode_index += 1;
    while(instrument->pqnode_index >= NUM_NODES) instrument->pqnode_index -= NUM_NODES;

    if(lpscheduler_get_now_seconds(&now) < 0) {
        syslog(LOG_CRIT, "SEQ PQ MSG: Error getting now in seq relay\n");
        return -1;
    }

    /* Hold on to the message as long as possible while still 
     * trying to leave some time for processing before the target deadline */
    seq_delay = instrument->msg.scheduled - (instrument->msg.max_processing_time * 2);
    d->timestamp = instrument->msg.initiated + seq_delay;

    // Remove the scheduled flag before relaying the message
    d->msg.flags &= ~LPFLAG_IS_SCHEDULED;

    if(pqueue_insert(instrument->msgpq, (void *)d) < 0) {
        syslog(LOG_ERR, "SEQ PQ MSG: Error while inserting message into pq during msgq loop: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

void * instrument_cleanup_thread(void * arg) {
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    /* free buffers that are done playing */
    while(instrument->is_running) {
        if(scheduler_cleanup_nursery(instrument->async_mixer) < 0) {
            syslog(LOG_ERR, "%s cleanup thread: Could not cleanup nursery. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
        }
        usleep((useconds_t)10000);
    }

    if(scheduler_cleanup_nursery(instrument->async_mixer) < 0) {
        syslog(LOG_ERR, "%s cleanup thread: Could not cleanup nursery. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
    }

    return NULL;
}

void * instrument_message_thread(void * arg) {
    lpmsg_t bufmsg = {0}; // the message serialized along with the async buffer...
    lpbuffer_t * buf; // async renders: FIXME, do renders in a thread if possible... or fork out early for the python interpreter maybe?
    //double processing_time_so_far, onset_delay_in_seconds, now=0;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;
    int is_scheduled = 0;

    instrument->is_waiting = 1;
    while(instrument->is_running) {
        if(astrid_msgq_read(instrument->msgq, &instrument->msg) == (mqd_t) -1) {
            syslog(LOG_ERR, "%s renderer: Could not read message from playq. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
            usleep((useconds_t)10000);
            return 0;
        }

        is_scheduled = ((instrument->msg.flags & LPFLAG_IS_SCHEDULED) == LPFLAG_IS_SCHEDULED);
        syslog(LOG_DEBUG, "C MSG: name=%s\n", instrument->msg.instrument_name);
        syslog(LOG_DEBUG, "C MSG: scheduled=%f\n", instrument->msg.scheduled);
        syslog(LOG_DEBUG, "C MSG: voice_id=%d\n", (int)instrument->msg.voice_id);
        syslog(LOG_DEBUG, "C MSG: type=%d\n", (int)instrument->msg.type);
        syslog(LOG_DEBUG, "C MSG: flags=%d\n", (int)instrument->msg.flags);
        syslog(LOG_DEBUG, "C MSG: is_scheduled=%d\n", is_scheduled);

        // Handle shutdown early
        if(instrument->msg.type == LPMSG_SHUTDOWN) {
            syslog(LOG_DEBUG, "C MSG: shutdown\n");
            instrument->is_running = 0;

            // send the shutdown message to the seq thread and external relay
            if(relay_message_to_seq(instrument) < 0) {
                syslog(LOG_ERR, "%s renderer: Could not read relay message to seq. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
            }
            if(instrument->ext_relay_enabled) {
                if(send_message(instrument->external_relay_name, instrument->msg) < 0) {
                    syslog(LOG_ERR, "Could not relay message\n");
                }
            }

            if(send_serial_message(instrument->msg) < 0) {
                syslog(LOG_ERR, "Could not relay serial message\n");
            }

            break;
        }

        if(is_scheduled) {
            // Scheduled messages get sent to the sequencer for handling later
            syslog(LOG_ERR, "C IS SCHEDULED msg.scheduled %f\n", instrument->msg.scheduled);
            if(relay_message_to_seq(instrument) < 0) {
                syslog(LOG_ERR, "%s renderer: Could not read relay message to seq. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
            }
            continue;
        } else {
            // All other messages get relayed externally, too, if the relay is enabled
            if(instrument->ext_relay_enabled) {
                syslog(LOG_DEBUG, "C MSG: relaying to ext\n");
                if(send_message(instrument->external_relay_name, instrument->msg) < 0) {
                    syslog(LOG_ERR, "Could not relay message\n");
                }
            }
        }

        // Now the fun stuff
        switch(instrument->msg.type) {
            case LPMSG_RENDER_COMPLETE:
                /* FIXME do this in another thread? */
                // Renders from the internal callback AND/OR external renderers (AKA python)
                if((buf = deserialize_buffer(instrument->msg.msg, &bufmsg)) == NULL) {
                    syslog(LOG_ERR, "DAC could not deserialize buffer. Error: (%d) %s\n", errno, strerror(errno));
                    continue;
                }

                // tracking the render time is still useful... this might not be right place tho
                /*
                if(lpscheduler_get_now_seconds(&now) < 0) {
                    syslog(LOG_ERR, "Could not get now seconds for loop retriggering\n");
                    now = 0;
                }

                processing_time_so_far = now - instrument->msg.initiated;
                onset_delay_in_seconds = instrument->msg.scheduled - processing_time_so_far;
                if(onset_delay_in_seconds < 0) onset_delay_in_seconds = 0.f;

                instrument->msg.onset_delay = (size_t)(onset_delay_in_seconds * ASTRID_SAMPLERATE);

                syslog(LOG_INFO, "msg.onset_delay %ld\n", instrument->msg.onset_delay);
                */

                /* Schedule the buffer for playback */
                syslog(LOG_INFO, "RENDER COMPLETE: scheduling buffer with value 10 %f\n", buf->data[10]);
                scheduler_schedule_event(instrument->async_mixer, buf, 0);
                //scheduler_debug(instrument->async_mixer);
                break;

            case LPMSG_UPDATE:
                syslog(LOG_DEBUG, "C MSG: update\n");
                if(instrument->update == NULL) continue;
                // decode each update param and call the callback with it
                if(process_param_updates(instrument) < 0) {
                    syslog(LOG_ERR, "Could not encode update messages...\n");
                    continue;
                }
                //instrument->update(instrument);
                break;

            case LPMSG_PLAY:
                syslog(LOG_DEBUG, "C MSG: play\n");
                // Schedule a C callback render if there's a callback defined
                // python renders will also be triggered at this point if we're 
                // inside a python instrument because of the message relay
                if(instrument->renderer != NULL) {
                    syslog(LOG_DEBUG, "C MSG: rendering play...\n");
                    /* FIXME do this in another thread */
                    if(instrument->renderer(instrument) < 0) {
                        syslog(LOG_ERR, "there was a problem rendering from the C instrument\n");
                        continue;
                    }
                }
                break;

            case LPMSG_SERIAL:
                syslog(LOG_DEBUG, "C MSG: serial\n");
                // serial messages get relayed from here to the special serial Q, 
                // and then written out to the tty in the serial listener thread...
                if(send_serial_message(instrument->msg) < 0) {
                    syslog(LOG_ERR, "Could not send serial message...\n");
                }
                break;

            case LPMSG_LOAD:
                // it would be interesting to explore live reloading of C modules
                // in the tradition of CLIVE, but at the moment only python handles these
                syslog(LOG_DEBUG, "C MSG: load\n");
                break;

            case LPMSG_TRIGGER:
                // maybe support raspberry pi GPIO pin toggling / triggers?
                syslog(LOG_DEBUG, "C MSG: trigger\n");
                if(instrument->trigger != NULL) instrument->trigger(instrument);
                break;

            case LPMSG_MIDI_FROM_DEVICE:
                syslog(LOG_DEBUG, "C MSG: midi from device\n");
                break;

            case LPMSG_MIDI_TO_DEVICE:
                syslog(LOG_DEBUG, "C MSG: midi to device\n");
                break;

            default:
                // Garbage typed messages will shut 'er down
                syslog(LOG_WARNING, "C MSG: got bad msg type %d\n", instrument->msg.type);
                break;
        }
    }

    instrument->is_waiting = 0;
    return 0;
}

int astrid_instrument_seq_start(lpinstrument_t * instrument) {
    int i;

    /* Allocate the pq message nodes */
    if((instrument->pqnodes = (lpmsgpq_node_t *)calloc(NUM_NODES, sizeof(lpmsgpq_node_t))) == NULL) {
        syslog(LOG_ERR, "Could not initialize message priority queue nodes. Error: %s\n", strerror(errno));
        return -1;
    }
    memset(instrument->pqnodes, 0, NUM_NODES * sizeof(lpmsgpq_node_t));

    for(i=0; i < NUM_NODES; i++) {
        instrument->pqnodes[i].index = i;
    }

    /* Create the message priority queue */
    if((instrument->msgpq = pqueue_init(NUM_NODES, msgpq_cmp_pri, msgpq_get_pri, msgpq_set_pri, msgpq_get_pos, msgpq_set_pos)) == NULL) {
        syslog(LOG_ERR, "Could not initialize message priority queue. Error: %s\n", strerror(errno));
        return -1;
    }

    /* Start message pq thread */
    if(pthread_create(&instrument->message_scheduler_pq_thread, NULL, instrument_seq_pq, (void*)instrument) != 0) {
        syslog(LOG_ERR, "Could not initialize message scheduler pq thread. Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

#if 0
// This was fun to write but...
int encode_serial_msg(lpmsg_t * msg) {
    char c=0, lastc=SPACE;
    char token[100] = {0}; // 100 chars is prolly enough for single precision floats
    int scan_pos=0, token_chars_read=0, token_count=0;
    uint8_t type=0, id=0;
    float value = 0.f;
    lpserialmsg_t smsg;

    syslog(LOG_DEBUG, "Encoding msg.msg `%s` for serial transmission!\n", msg->msg);

    int num_tokens = 3;
    int parse_order[3] = {
        LPPARAM_STRING, // The message type
        LPPARAM_INT,    // The id
        LPPARAM_FLOAT   // The value
    };

    c = msg->msg[scan_pos];
    while(scan_pos < (int)LPMAXMSG && token_count < num_tokens) {
        scan_pos += 1;

        /* Prevent overflow on long-ass tokens */
        if(token_chars_read >= 100) continue;

        if((c == SPACE || c == '\0') && lastc != SPACE) {
            token[token_chars_read] = '\0';
            switch(parse_order[token_count]) {
                case LPPARAM_STRING:
                    // just look at the first char to determine the message type
                    switch(token[0]) {
                        case 'm':
                            type = SMSG_MOTOR_SPEED;
                            break;

                        case 's':
                            type = SMSG_SOLENOID_TRIGGER;
                            break;

                        default:
                            type = SMSG_UNKNOWN;
                            break;
                    }
                    break;

                case LPPARAM_INT:
                    id = atoi(token);
                    break;

                case LPPARAM_FLOAT:
                    value = atof(token);
                    break;

                default:
                    break;
            }

            token_chars_read = 0;
            token_count += 1;
        } else if(c != SPACE) {
            token[token_chars_read] = c;
            token_chars_read += 1;
        }

        if(c == '\0') break;
        lastc = c;
        c = msg->msg[scan_pos];
    }

    smsg.type = type;
    smsg.id = id;
    smsg.value = value;

    syslog(LOG_DEBUG, "PARSER constructed message: type=%d id=%d value=%f\n", type, id, value);

    if(memset(&msg->msg, 0, LPMAXMSG) == NULL) {
        syslog(LOG_ERR, "%s encode_serial_msg: memset failure. Error: (%d) %s\n", msg->instrument_name, errno, strerror(errno));
        return -1;
    }

    if(memcpy(&msg->msg, &smsg, sizeof(lpserialmsg_t)) == NULL) {
        syslog(LOG_ERR, "%s encode_serial_msg: memcpy failure. Error: (%d) %s\n", msg->instrument_name, errno, strerror(errno));
        return -1;
    }

    return 0;
}
#endif

void log_byte_buffer(unsigned char * buf, size_t size) {
    size_t i;
    for(i=0; i < size; i++) {
        if(buf[i] == '\n') {
            syslog(LOG_DEBUG, "%ld: \\n\n", i);
        } else if(buf[i] == '\r') {
            syslog(LOG_DEBUG, "%ld: \\r\n", i);
        } else if(buf[i] == '\t') {
            syslog(LOG_DEBUG, "%ld: \\t\n", i);
        } else if(isprint(buf[i])) {
            syslog(LOG_DEBUG, "%ld: %c\n", i, buf[i]);
        } else {
            syslog(LOG_DEBUG, "%ld: \\x%02X\n", i, buf[i]);
        }
    }
}

void * instrument_serial_listener_thread(void * arg) {
    struct termios options;
    unsigned char serialmsgbuf[sizeof(lpserialmsg_t)] = {0};
    lpserialmsg_t header = {0};
    unsigned char * payload;
    unsigned char c;
    unsigned char ready = 'c';
    lpmsg_t msg = {0};
    fd_set read_fds;
    int tty, biggest_fd, bytes_written, bytes_read, tty_is_ready=0;
    size_t bytes_in_buf=0;
    lpinstrument_t * instrument = (lpinstrument_t *)arg;

    if(!instrument->tty_is_enabled) {
        syslog(LOG_INFO, "%s serial listener: no tty requested, shutting down here!\n", instrument->name);
        return NULL;
    }

    // Open the tty 
    // TODO support multiple ttys, duuuh
    tty = open(instrument->tty_path, O_RDWR | O_NOCTTY);
    if(tty < 0) {
        syslog(LOG_ERR, "Problem connecting to TTY -- shutting down serial listener thread...\n");
        return NULL;
    }

    tcgetattr(tty, &options);
    //cfsetispeed(&options, B1000000);
    //cfsetospeed(&options, B1000000);
    cfmakeraw(&options);
    tcsetattr(tty, TCSANOW, &options);

    while(!tty_is_ready) {
        // signal the tty we've connected
        bytes_written = write(tty, &ready, 1);
        if(bytes_written != 1) {
            syslog(LOG_ERR, "%s serial listener: unexpected number of bytes written to tty. (%d) Error: (%d) %s\n", instrument->name, bytes_written, errno, strerror(errno));
            return NULL;
        }

        bytes_read = read(tty, &c, 1);
        syslog(LOG_ERR, "%s serial listener: got %d byte: %c. ready for messages.\n", instrument->name, bytes_read, c);
        if(bytes_read < 0) {
            syslog(LOG_ERR, "%s serial listener: Could not read header from the tty. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
            return NULL;
        } 
        if(c == 'c') {
            syslog(LOG_ERR, "%s serial listener: got ready byte %c\n", instrument->name, c);
            tty_is_ready = 1;
        }
    }

    while(instrument->is_running) {
        // FIXME before calling select, check the session for new / removed ttys and close 
        // or open their handles for reading/writing, setting them each in the fds collections.
        // Also keep an array of tty_is_ready[1024] mapped fd=readyval since select only works 
        // with max 1024 file descriptors anyway. And adapt the biggest fd calc on each loop...
        
        FD_ZERO(&read_fds);
        FD_SET(instrument->serialmsgq, &read_fds);
        FD_SET(tty, &read_fds);

        // nfds should be the biggest fd + 1 ...
        biggest_fd = (tty > instrument->serialmsgq ? tty : instrument->serialmsgq) + 1;

        // Wait forever until something is ready for reading or writing
        if(select(biggest_fd, &read_fds, NULL, NULL, NULL) < 0) {
            syslog(LOG_ERR, "%s serial listener: select. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
            usleep((useconds_t)10000);
            continue;
        }

        // read some bytes from the tty if there are some
        if(FD_ISSET(tty, &read_fds)) {
            // Read the header
            bytes_in_buf = 0;
            while(bytes_in_buf < sizeof(lpserialmsg_t)) {
                bytes_read = read(tty, serialmsgbuf+bytes_in_buf, sizeof(lpserialmsg_t)-bytes_in_buf);
                //syslog(LOG_ERR, "%s serial listener: got header bytes %d\n", instrument->name, bytes_read);
                //log_byte_buffer(serialmsgbuf, sizeof(lpserialmsg_t));
                if(bytes_read < 0) {
                    syslog(LOG_ERR, "%s serial listener: Could not read header from the tty. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
                    goto handle_serial_message_events;
                } 
                bytes_in_buf += bytes_read; 
            }

            memcpy(&header, serialmsgbuf, sizeof(lpserialmsg_t));
            if(header.size > 0) {
                payload = (unsigned char *)malloc(header.size);
                bytes_in_buf = 0;
                while(bytes_in_buf < header.size) {
                    bytes_read = read(tty, payload+bytes_in_buf, header.size-bytes_in_buf);
                    //syslog(LOG_ERR, "%s serial listener: got payload bytes %d\n", instrument->name, bytes_read);
                    //log_byte_buffer(payload, header.size);

                    if(bytes_read < 0) {
                        syslog(LOG_ERR, "%s serial listener: Could not read payload from the tty. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
                        free(payload);
                        goto handle_serial_message_events;
                    } 
                    bytes_in_buf += bytes_read;
                }
            }

            // parse and handle the msg:
            // write the payload into the session, and the payload ID into the msg_t
            // Convert the message to msg_t update
            memset(&msg, 0, sizeof(lpmsg_t));
            msg.type = header.type;
            memcpy(msg.instrument_name, header.instrument_name, LPMAXNAME);

            if(header.size > 0 && header.type == LPMSG_UPDATE) {
                // Update messages have a fixed size ID/value payload
                memcpy(msg.msg, payload, header.size);
                free(payload);
            } else if(header.size > 0 && header.type == LPMSG_DATA) {
                // Data messages get their payloads stored in shared memory (with a wrapper, probably?)
                free(payload);
            }
            // Send it on the instrument msg queue
            send_play_message(msg);

            // when we're done reading the message, clear the buffer & header
            memset(&serialmsgbuf, 0, sizeof(serialmsgbuf));
            memset(&header, 0, sizeof(lpserialmsg_t));
            bytes_in_buf = 0;
        }

handle_serial_message_events:
        // Handle messages on the serial queue if there are any
        if(FD_ISSET(instrument->serialmsgq, &read_fds)) {
            // if there's a message on the queue, write it to the tty
            if(astrid_msgq_read(instrument->serialmsgq, &msg) == (mqd_t) -1) {
                syslog(LOG_ERR, "%s serial listener: Could not read message from the q. Error: (%d) %s\n", instrument->name, errno, strerror(errno));
                usleep((useconds_t)10000);
                continue;
            }

            if(msg.type == LPMSG_SHUTDOWN) break;

            syslog(LOG_DEBUG, "Got a message to relay over serial! Writing it to the tty...\n");

            // FIXME check write_fds here, and queue messages for writing later if 
            // it's not possible to write (or just drop them I guess?)
            bytes_written = write(tty, &msg.msg, sizeof(lpserialmsg_t));
            if(bytes_written != sizeof(lpserialmsg_t)) {
                syslog(LOG_ERR, "%s serial listener: unexpected number of bytes written to tty. (%d) Error: (%d) %s\n", instrument->name, bytes_written, errno, strerror(errno));
                usleep((useconds_t)10000);
                continue;
            }

            syslog(LOG_DEBUG, "Wrote %d bytes to the tty...\n", bytes_written);
        }
    }

    syslog(LOG_INFO, "%s serial listener: shutting down here!\n", instrument->name);
    close(tty);

    return 0;
}

void * instrument_midi_listener_thread(void * arg) {
    snd_seq_t * seq_handle;
    snd_seq_event_t * event;
    int ret, port;

    lpinstrument_t * instrument = (lpinstrument_t *)arg;

    if((ret = snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0)) < 0) {
        syslog(LOG_ERR, "%s midi listener: Could not open ALSA seq. Error: (%d) %s\n", instrument->name, ret, snd_strerror(ret));
        return 0;
    }

    snd_seq_set_client_name(seq_handle, "astrid_midi_listener");

    if((port = snd_seq_create_simple_port(seq_handle, "input", 
                    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, 
                    SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
        syslog(LOG_ERR, "%s midi listener: Could not create ALSA port. Error: (%d) %s\n", instrument->name, port, snd_strerror(port));
        snd_seq_close(seq_handle);
        return 0;
    }

    if((ret = snd_seq_connect_from(seq_handle, port, 24, 0)) < 0) {
        syslog(LOG_ERR, "%s midi listener: Could not connect to ALSA port. Error: (%d) %s\n", instrument->name, ret, snd_strerror(ret));
        snd_seq_close(seq_handle);
        return 0;
    }

    while(instrument->is_running) {
        snd_seq_event_input(seq_handle, &event);
        switch(event->type) {
            case SND_SEQ_EVENT_NOTEON:
                lpmidi_relay_to_instrument(instrument->name, NOTE_ON, event->data.note.note, event->data.note.velocity);
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                lpmidi_relay_to_instrument(instrument->name, NOTE_OFF, event->data.note.note, event->data.note.velocity);
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                lpmidi_relay_to_instrument(instrument->name, CONTROL_CHANGE, event->data.control.param, event->data.control.value);
                break;
            default:
                break;
        }
    }

    snd_seq_close(seq_handle);
    return 0;
}

lpinstrument_t * astrid_instrument_start(
    char * name, 
    int channels, 
    int ext_relay_enabled,
    double adc_length,
    void * ctx,
    char * tty,
    int (*stream)(size_t blocksize, float ** input, float ** output, void * instrument),
    int (*renderer)(void * instrument),
    int (*update)(void * instrument, char * key, char * val),
    int (*trigger)(void * instrument)
) {
    lpinstrument_t * instrument;
    struct sigaction shutdown_action;
    jack_status_t jack_status;
    jack_options_t jack_options = JackNullOption;
    const char ** ports;
    char outport_name[50];
    char inport_name[50];
    int c = 0;

    instrument = (lpinstrument_t *)LPMemoryPool.alloc(1, sizeof(lpinstrument_t));
    memset(instrument, 0, sizeof(lpinstrument_t));

    syslog(LOG_DEBUG, "starting %s instrument...\n", name);

    instrument->name = name;
    instrument->channels = channels;
    instrument->context = ctx;

    instrument->stream = stream;
    instrument->renderer = renderer;
    instrument->update = update;
    instrument->trigger = trigger;
    instrument->ext_relay_enabled = ext_relay_enabled;

    openlog(name, LOG_PID, LOG_USER);

    /* Seed the random number generator */
    LPRand.preseed();

    /* Set shutdown signal handlers */
    shutdown_action.sa_handler = handle_instrument_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = SA_RESTART; /* Prevent open, read, write etc from EINTR */
    astrid_instrument_is_running = &instrument->is_running;

    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "%s Could not init SIGINT signal handler. Error: %s\n", name, strerror(errno));
        exit(1);
    }

    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        syslog(LOG_ERR, "%s Could not init SIGTERM signal handler. Error: %s\n", name, strerror(errno));
        exit(1);
    }

    // Set the message q names
    snprintf(instrument->qname, NAME_MAX, "/%s-msgq", instrument->name);
    snprintf(instrument->serial_message_q_name, NAME_MAX, "/%s-serial-msgq", instrument->name);

    // Set the tty path
    if(tty != NULL) {
        snprintf(instrument->tty_path, NAME_MAX, "%s", tty);
        instrument->tty_is_enabled = 1;
    }

    if(instrument->ext_relay_enabled) {
        snprintf(instrument->external_relay_name, NAME_MAX, "/%s-extrelay-msgq", instrument->name);
    } 

    // set up the voice counter and the buffer counter
    if(lpcounter_create("voiceid") < 0) {
        syslog(LOG_ERR, "%s Could not create voiceid counter. Error: %s\n", name, strerror(errno));
        exit(1);
    }
    if(lpcounter_create("bufferid") < 0) {
        syslog(LOG_ERR, "%s Could not create bufferid counter. Error: %s\n", name, strerror(errno));
        exit(1);
    }

    /* Open the LMDB session */
    astrid_instrument_session_open(instrument);

    /* Set up JACK */
    instrument->inports = (jack_port_t **)calloc(channels, sizeof(jack_port_t *));
    instrument->outports = (jack_port_t **)calloc(channels, sizeof(jack_port_t *));
    instrument->jack_client = jack_client_open(name, jack_options, &jack_status, NULL);

    syslog(LOG_DEBUG, "JACK STATUS %d\n", (int)jack_status);

    if(instrument->jack_client == NULL) {
        syslog(LOG_ERR, "%s Could not open jack client. Client is NULL: %s\n", name, strerror(errno));
        if((jack_status & JackServerFailed) == JackServerFailed) {
            syslog(LOG_ERR, "%s Could not open jack client. Jack server failed with status %2.0x\n", name, jack_status);
        } else {
            syslog(LOG_ERR, "%s Could not open jack client. Unknown error: %s\n", name, strerror(errno));
        }
        goto astrid_instrument_shutdown_with_error;
    }

    if((jack_status & JackServerStarted) == JackServerStarted) {
        syslog(LOG_INFO, "Jack server started!\n");
    }

    /* Get the samplerate from JACK */
    /* FIXME could use the JACK D-Bus API here to let instruments change the samplerate? */
    instrument->samplerate = (lpfloat_t)jack_get_sample_rate(instrument->jack_client);

    /* Create the internal ringbuffer for sampling from the adc */
    snprintf(instrument->adcname, PATH_MAX, "%s-adc", instrument->name);
    if((instrument->adcbuf = lpsampler_create(instrument->adcname, adc_length, instrument->channels, instrument->samplerate)) == NULL) {
        syslog(LOG_INFO, "Could not create instrument ADC buffer\n");
        goto astrid_instrument_shutdown_with_error;
    }

    /* init scheduler */
    instrument->async_mixer = scheduler_create(1, instrument->channels, instrument->samplerate);

    /* Set the main jack callback which always runs: maybe there is an analysis-only use to support too? */
    jack_set_process_callback(instrument->jack_client, astrid_instrument_jack_callback, (void *)instrument);
    for(c=0; c < channels; c++) {
        snprintf(outport_name, sizeof(outport_name), "out%d", c);
        instrument->outports[c] = jack_port_register(instrument->jack_client, outport_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

        snprintf(inport_name, sizeof(inport_name), "in%d", c);
        instrument->inports[c] = jack_port_register(instrument->jack_client, inport_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }

    /* Request some ports from jack */
    for(c=0; c < channels; c++) {
        if(instrument->outports[c] == NULL) {
            syslog(LOG_ERR, "No more JACK output ports available, shutting down...\n");
            goto astrid_instrument_shutdown_with_error;
        }

        if(instrument->inports[c] == NULL) {
            syslog(LOG_ERR, "No more JACK input ports available, shutting down...\n");
            goto astrid_instrument_shutdown_with_error;
        }
    }

    /* activate the jack client */
    if(jack_activate(instrument->jack_client) != 0) {
        syslog(LOG_ERR, "%s Could not activate JACK client, shutting down...\n", name);
        goto astrid_instrument_shutdown_with_error;
    }

    /* connect the jack ports FIXME would be nice to optionally do routing in the instrument callback */
    if((ports = jack_get_ports(instrument->jack_client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
		syslog(LOG_CRIT, "Cannot find any physical capture ports");
        goto astrid_instrument_shutdown_with_error;
	}

    for(c=0; c < instrument->channels; c++) {
        if(jack_connect(instrument->jack_client, ports[c], jack_port_name(instrument->inports[c]))) {
            syslog(LOG_NOTICE, "Could not connect input port %d. (There probably aren't enough available inputs on this device.)\n", c);
        }
    }
	free(ports);
	
	if((ports = jack_get_ports(instrument->jack_client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
		syslog(LOG_CRIT, "Cannot find any physical playback ports");
        goto astrid_instrument_shutdown_with_error;
	}

    for(c=0; c < instrument->channels; c++) {
        if(jack_connect(instrument->jack_client, jack_port_name(instrument->outports[c]), ports[c])) {
            syslog(LOG_NOTICE, "Could not connect output port %d. (There probably aren't enough available outputs on this device.)\n", c);
        }
    }
	free(ports);

    /* Register that everything is running and ready */
    instrument->is_running = 1;
    syslog(LOG_INFO, "%s is running...\n", name);

    /* Open the message queue */
    if((instrument->msgq = astrid_msgq_open(instrument->qname)) == (mqd_t) -1) {
        syslog(LOG_CRIT, "Could not open msgq for instrument %s. Error: %s\n", instrument->name, strerror(errno));
        return NULL;
    }
    syslog(LOG_DEBUG, "Opened message queue for %s with fd %d\n", instrument->name, instrument->msgq);

    if((instrument->serialmsgq = astrid_msgq_open(instrument->serial_message_q_name)) == (mqd_t) -1) {
        syslog(LOG_CRIT, "Could not open serial msgq for instrument %s. Error: %s\n", instrument->name, strerror(errno));
        return NULL;
    }
    syslog(LOG_DEBUG, "Opened serial message queue for %s with fd %d\n", instrument->name, instrument->serialmsgq);

    if(instrument->ext_relay_enabled) {
        if((instrument->exmsgq = astrid_msgq_open(instrument->external_relay_name)) == (mqd_t) -1) {
            syslog(LOG_CRIT, "Could not open external message relay for instrument %s. Error: %s\n", instrument->name, strerror(errno));
            return NULL;
        }
        syslog(LOG_DEBUG, "Opened message relay queue for %s with fd %d\n", instrument->name, instrument->exmsgq);
    }

    /* Prepare the message structs */ 
    init_instrument_message(&instrument->msg, instrument->name);
    init_instrument_message(&instrument->cmd, instrument->name);

    /* Start the sequencer thread */
    if(astrid_instrument_seq_start(instrument) < 0) {
        syslog(LOG_CRIT, "Could not start message sequence threads for instrument %s. Error: %s\n", instrument->name, strerror(errno));
        return NULL;
    }

    /* Start listening for messages in the message feed thread */
    if(pthread_create(&instrument->message_feed_thread, NULL, instrument_message_thread, (void*)instrument) != 0) {
        syslog(LOG_ERR, "Could not initialize instrument message thread. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* Start listening for MIDI messages in the midi listener thread */
    // TODO support multiple devices
    if(pthread_create(&instrument->midi_listener_thread, NULL, instrument_midi_listener_thread, (void*)instrument) != 0) {
        syslog(LOG_ERR, "Could not initialize instrument midi listener thread. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* start the cleanup thread */
    if(pthread_create(&instrument->cleanup_thread, NULL, instrument_cleanup_thread, (void*)instrument) != 0) {
        syslog(LOG_ERR, "Could not initialize instrument cleanup thread. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* start the serial listener thread */
    // TODO support multiple devices -- maybe just register them in the session?
    if(pthread_create(&instrument->serial_listener_thread, NULL, instrument_serial_listener_thread, (void*)instrument) != 0) {
        syslog(LOG_ERR, "Could not initialize serial listener thread. Error: %s\n", strerror(errno));
        return NULL;
    }

    /* setup linenoise repl */
    linenoiseHistoryLoad("history.txt"); // FIXME this goes in the instrument config dir / or share?

    return instrument;

astrid_instrument_shutdown_with_error:
    instrument->is_running = 0;
    jack_client_close(instrument->jack_client);
    closelog();
    return NULL;
}

int astrid_instrument_stop(lpinstrument_t * instrument) {
    int c, ret;

    syslog(LOG_INFO, "%s instrument shutting down and cleaning up...\n", instrument->name);
    syslog(LOG_INFO, "Sending shutdown message to threads and queues...\n");
    if(instrument->is_waiting) {
        instrument->msg.type = LPMSG_SHUTDOWN;
        if(send_play_message(instrument->msg) < 0) {
            fprintf(stderr, "astrid instrument message thread cleanup: Could not send shutdown message...\n");
        }
    }

    syslog(LOG_DEBUG, "Joining with message thread...\n");
    if((ret = pthread_join(instrument->message_feed_thread, NULL)) != 0) {
        if(ret == EINVAL) syslog(LOG_ERR, "EINVAL\n");
        if(ret == EDEADLK) syslog(LOG_ERR, "DEADLOCK\n");
        if(ret == ESRCH) syslog(LOG_ERR, "ESRCH\n");
        syslog(LOG_ERR, "Error while attempting to join with message feed thread. Ret: %d Errno: %d (%s)\n", ret, errno, strerror(ret));
    }

    syslog(LOG_DEBUG, "Joining with message scheduler pq thread...\n");
    if((ret = pthread_join(instrument->message_scheduler_pq_thread, NULL)) != 0) {
        if(ret == EINVAL) syslog(LOG_ERR, "EINVAL\n");
        if(ret == EDEADLK) syslog(LOG_ERR, "DEADLOCK\n");
        if(ret == ESRCH) syslog(LOG_ERR, "ESRCH\n");
        syslog(LOG_ERR, "Error while attempting to join with message scheduler pq thread. Ret: %d Errno: %d (%s)\n", ret, errno, strerror(ret));
    }

    syslog(LOG_DEBUG, "Joining with cleanup thread...\n");
    if((ret = pthread_join(instrument->cleanup_thread, NULL)) != 0) {
        if(ret == EINVAL) syslog(LOG_ERR, "EINVAL\n");
        if(ret == EDEADLK) syslog(LOG_ERR, "DEADLOCK\n");
        if(ret == ESRCH) syslog(LOG_ERR, "ESRCH\n");
        syslog(LOG_ERR, "Error while attempting to join with cleanup thread. Ret: %d Errno: %d (%s)\n", ret, errno, strerror(ret));
    }

    syslog(LOG_DEBUG, "Joining with midi listener thread...\n");
    if((ret = pthread_join(instrument->midi_listener_thread, NULL)) != 0) {
        if(ret == EINVAL) syslog(LOG_ERR, "EINVAL\n");
        if(ret == EDEADLK) syslog(LOG_ERR, "DEADLOCK\n");
        if(ret == ESRCH) syslog(LOG_ERR, "ESRCH\n");
        syslog(LOG_ERR, "Error while attempting to join with midi listener thread. Ret: %d Errno: %d (%s)\n", ret, errno, strerror(ret));
    }

    syslog(LOG_DEBUG, "Joining with serial listener thread...\n");
    if((ret = pthread_join(instrument->serial_listener_thread, NULL)) != 0) {
        if(ret == EINVAL) syslog(LOG_ERR, "EINVAL\n");
        if(ret == EDEADLK) syslog(LOG_ERR, "DEADLOCK\n");
        if(ret == ESRCH) syslog(LOG_ERR, "ESRCH\n");
        syslog(LOG_ERR, "Error while attempting to join with serial listener thread. Ret: %d Errno: %d (%s)\n", ret, errno, strerror(ret));
    }

    syslog(LOG_DEBUG, "Closing instrument message queue...\n");
    if(instrument->msgq != (mqd_t) -1) astrid_msgq_close(instrument->msgq);

    syslog(LOG_DEBUG, "Closing serial message queue...\n");
    if(instrument->serialmsgq != (mqd_t) -1) astrid_msgq_close(instrument->serialmsgq);

    syslog(LOG_DEBUG, "Closing external message queue...\n");
    if(instrument->exmsgq != (mqd_t) -1) astrid_msgq_close(instrument->exmsgq);

    syslog(LOG_DEBUG, "Stopping JACK...\n");
    for(c=0; c < instrument->channels; c++) {
        jack_port_unregister(instrument->jack_client, instrument->outports[c]);
        jack_port_unregister(instrument->jack_client, instrument->inports[c]);
    }

    jack_client_close(instrument->jack_client);

    /* and the jack i/o buffers */
    free(instrument->inports);
    free(instrument->outports);

    syslog(LOG_DEBUG, "Closing lmdb session...\n");
    astrid_instrument_session_close(instrument);

    /* save the history FIXME save the path on the instrument */
    linenoiseHistorySave("history.txt");

    if(instrument->async_mixer != NULL) scheduler_destroy(instrument->async_mixer);

    syslog(LOG_DEBUG, "Cleaning up adc ringbuf...\n");
    if(lpsampler_destroy(instrument->adcname) < 0) {
        syslog(LOG_ERR, "Error while removing adc ringbuf, dang! Other cleanup is done tho.\n");
        closelog();
        return -1;
    }

    /* cleanup the pq memory */
    pqueue_free(instrument->msgpq);
    free(instrument->pqnodes);

    /* poof! */
    free(instrument);

    syslog(LOG_DEBUG, "All done, see ya later!\n");
    closelog();
    return 0;
}

int astrid_instrument_get_or_create_datadir(const char * name, char * dbpath) {
    int ret;
    char astrid_data_path[PATH_MAX];
    char xdg_data_home[PATH_MAX];
    char * user_home;
    char * _xdg_data_home = getenv("XDG_DATA_HOME");
    char * default_data_dir = ".local/share";
    char * instrument_subdir = "astrid_instruments";

    if(_xdg_data_home == NULL) {
        user_home = getenv("HOME");
        ret = snprintf(xdg_data_home, PATH_MAX, "%s/%s", user_home, default_data_dir);
        if(ret < 0) {
            syslog(LOG_ERR, "astrid data path (%s) snprintf: (%d) %s\n", default_data_dir, errno, strerror(errno));
            return -1;
        }

    } else {
        memcpy(xdg_data_home, _xdg_data_home, PATH_MAX);
    }

    if(access(xdg_data_home, F_OK) != 0) {
        /* create the xdg data home dir */
        if(mkdir(xdg_data_home, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            syslog(LOG_ERR, "session data path (%s) mkdir: (%d) %s\n", xdg_data_home, errno, strerror(errno));
            return -1;
        }
    }

    ret = snprintf(astrid_data_path, PATH_MAX, "%s/%s", xdg_data_home, instrument_subdir);
    if(ret < 0) {
        syslog(LOG_ERR, "astrid data path (%s) snprintf: (%d) %s\n", astrid_data_path, errno, strerror(errno));
        return -1;
    }

    if(access(astrid_data_path, F_OK) != 0) {
        /* create the astrid data path */
        if(mkdir(astrid_data_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            syslog(LOG_ERR, "astrid data path (%s) mkdir: (%d) %s\n", astrid_data_path, errno, strerror(errno));
            return -1;
        }
    }

    ret = snprintf(dbpath, PATH_MAX, "%s/%s", astrid_data_path, name);
    if(ret < 0) {
        syslog(LOG_ERR, "astrid data path (%s) snprintf: (%d) %s\n", astrid_data_path, errno, strerror(errno));
        return -1;
    }

    if(access(dbpath, F_OK) != 0) {
        /* create the data dir */
        if(mkdir(dbpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            syslog(LOG_ERR, "session data path (%s) mkdir: (%d) %s\n", dbpath, errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int astrid_instrument_session_open(lpinstrument_t * instrument) {
    int rc;

    if(astrid_instrument_get_or_create_datadir(instrument->name, instrument->datapath) < 0) {
        syslog(LOG_ERR, "session data path (%s) mkdir: (%d) %s\n", instrument->datapath, errno, strerror(errno));
        return -1;
    }

    /* create the environment */
	rc = mdb_env_create(&instrument->dbenv);
    if(rc != MDB_SUCCESS) {
		syslog(LOG_ERR, "mdb_env_create: (%d) %s\n", rc, mdb_strerror(rc));
        return -1;
    }

    /* open it at the db directory */
	rc = mdb_env_open(instrument->dbenv, instrument->datapath, 0, 0664);
    if(rc != MDB_SUCCESS) {
        syslog(LOG_ERR, "mdb_env_open: (%d) %s\n", rc, mdb_strerror(rc));
        return -1;
    }

    /* begin a new read transaction */
	rc = mdb_txn_begin(instrument->dbenv, NULL, MDB_RDONLY, &instrument->dbtxn_read);
    if(rc != MDB_SUCCESS) {
        syslog(LOG_ERR, "mdb_txn_begin: read (%d) %s\n", rc, mdb_strerror(rc));
        return -1;
    }

    /* open the database inside the read transaction */
	rc = mdb_dbi_open(instrument->dbtxn_read, NULL, 0, &instrument->dbi);
    if(rc != MDB_SUCCESS) {
        syslog(LOG_ERR, "mdb_dbi_open: read (%d) %s\n", rc, mdb_strerror(rc));
        return -1;
    }

    /* begin a new read/write transaction */
	rc = mdb_txn_begin(instrument->dbenv, NULL, 0, &instrument->dbtxn_write);
    if(rc != MDB_SUCCESS) {
        syslog(LOG_ERR, "mdb_txn_begin: (%d) %s\n", rc, mdb_strerror(rc));
        return -1;
    }

    /* open the database inside the write transaction */
	rc = mdb_dbi_open(instrument->dbtxn_write, NULL, 0, &instrument->dbi);
    if(rc != MDB_SUCCESS) {
        syslog(LOG_ERR, "mdb_dbi_open: read (%d) %s\n", rc, mdb_strerror(rc));
        return -1;
    }

    /* finalize the transactions */
	mdb_txn_reset(instrument->dbtxn_read);
	mdb_txn_commit(instrument->dbtxn_write);

	return 0;
}

int astrid_instrument_session_close(lpinstrument_t * instrument) {
    syslog(LOG_DEBUG, "Closing LMDB session...\n");
    mdb_txn_abort(instrument->dbtxn_read);
	mdb_dbi_close(instrument->dbenv, instrument->dbi);
	mdb_env_close(instrument->dbenv);
    syslog(LOG_DEBUG, "Done cleaning up LMDB...\n");
    return 0;
}

int send_render_to_mixer(lpinstrument_t * instrument, lpbuffer_t * buf) {
    unsigned char * bufstr;
    size_t strsize = 0;

    syslog(LOG_INFO, "SEND RENDER serializing buffer with value 10 %f\n", buf->data[10]);

    if((bufstr = serialize_buffer(buf, &instrument->msg, &strsize)) == NULL) {
        return -1;
    }

    if(astrid_instrument_publish_bufstr(instrument->name, bufstr, strsize) < 0) {
        return -1;
    }

    free(bufstr);

    return 0;
}

int astrid_instrument_publish_bufstr(char * instrument_name, unsigned char * bufstr, size_t size) {
    int shmfd;
    void * shmaddr;
    char buffer_code[LPKEY_MAXLENGTH] = {0};
    ssize_t buffer_id = 0;
    lpmsg_t msg = {0};

    if((buffer_id = lpcounter_read_and_increment("bufferid")) < 0) {
        syslog(LOG_ERR, "Could not get bufferid. (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    // generate the buffer code using the instrument name as the prefix
    if(lpencode_with_prefix(instrument_name, buffer_id, buffer_code) < 0) {
        syslog(LOG_ERR, "Could not encode bufstr key. (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    // write the bufstr to shared memory at the key location
    /* Create the POSIX semaphore and initialize it to 1 */
    if(sem_open(buffer_code, O_CREAT | O_EXCL, LPIPC_PERMS, 1) == NULL) {
        syslog(LOG_ERR, "publish_bufstr: failed to create semaphore %s. Error: %s\n", buffer_code, strerror(errno));
        return -1;
    }

    /* Create the POSIX shared memory segment */
    if((shmfd = shm_open(buffer_code, O_CREAT | O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "publish_bufstr: Could not create shared memory segment. (%s) %s\n", buffer_code, strerror(errno));
        return -1;
    }

    if(ftruncate(shmfd, size) < 0) {
        syslog(LOG_ERR, "publish_bufstr: Could not truncate shared memory segment to size %ld. (%s) %s\n", size, buffer_code, strerror(errno));
        return -1;
    }

    /* Attach the shared memory to the pointer */
    if((shmaddr = (void*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "lpipc_buffer_aquire Could not mmap shared memory segment to size %ld. (%s) %s\n", size, buffer_code, strerror(errno));
        return -1;
    }

    /* Write the bufstr into the shared memory segment */
    memcpy(shmaddr, (void *)bufstr, size);

    // Send the render complete message
    memcpy(msg.instrument_name, instrument_name, strlen(instrument_name));
    memcpy(msg.msg, buffer_code, strlen(buffer_code));
    msg.type = LPMSG_RENDER_COMPLETE;
    if(send_play_message(msg) < 0) {
        syslog(LOG_ERR, "Could not send render complete message. (%d) %s\n", errno, strerror(errno));
        return 1;
    }

    // unmap the memory...
    munmap(shmaddr, size);

    close(shmfd);

    return 0;
}

int astrid_instrument_tick(lpinstrument_t * instrument) {
    char * cmdline;
    size_t cmdlength;

    if(instrument->is_running == 0) return 0;

    cmdline = linenoise("^_- ");

    if(cmdline == NULL) {
        return 0;
    }

    /* add the command to the history */
    linenoiseHistoryAdd(cmdline);

    cmdlength = strnlen(cmdline, ASTRID_MAX_CMDLINE);

    printf("msg: %s (%ld)\n", cmdline, cmdlength);

    if(parse_message_from_cmdline(cmdline, cmdlength, &instrument->cmd) < 0) {
        syslog(LOG_ERR, "Could not parse message from cmdline %s\n", cmdline);
        return -1;
    }

    free(cmdline);

    return 0;
}


int astrid_instrument_save_param_session_snapshot(lpinstrument_t * instrument, int num_params, int snapshot_id) {
    int shmfd;
    sem_t * sem;
    void * shm_base;
    size_t shm_size = 0;
    size_t offset = 0;
    char path[PATH_MAX] = {0};

    // Generate the path for the shared memory and semaphore
    snprintf(path, PATH_MAX, "%s-%s-%d", ASTRID_SESSION_SNAPSHOT_NAME, instrument->name, snapshot_id);

    // Create and initialize the semaphore
    if((sem = sem_open(path, O_CREAT, LPIPC_PERMS, 1)) == SEM_FAILED) {
        syslog(LOG_ERR, "Could not create semaphore. (%s) %s\n", path, strerror(errno));
        return -1;
    }

    // Calculate the required size for shared memory
    for(int i = 0; i < num_params; i++) {
        MDB_val key, data;
        key.mv_size = sizeof(int);
        key.mv_data = &i;

        int rc = mdb_txn_renew(instrument->dbtxn_read);
        if(rc == 0 && mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data) == 0) {
            shm_size += sizeof(int) + sizeof(size_t) + data.mv_size;
        }
        mdb_txn_reset(instrument->dbtxn_read);
    }

    // Create and truncate the shared memory segment
    if((shmfd = shm_open(path, O_CREAT | O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "Could not create shared memory segment. (%s) %s\n", path, strerror(errno));
        sem_close(sem);
        sem_unlink(path);
        return -1;
    }

    if(ftruncate(shmfd, shm_size) < 0) {
        syslog(LOG_ERR, "Could not truncate shared memory segment to size %ld. (%s) %s\n", shm_size, path, strerror(errno));
        close(shmfd);
        sem_close(sem);
        sem_unlink(path);
        return -1;
    }

    if((shm_base = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "Could not mmap shared memory segment. (%s) %s\n", path, strerror(errno));
        close(shmfd);
        sem_close(sem);
        sem_unlink(path);
        return -1;
    }

    // Lock the semaphore
    sem_wait(sem);

    // Write data to shared memory
    for(int i = 0; i < num_params; i++) {
        MDB_val key, data;
        key.mv_size = sizeof(int);
        key.mv_data = &i;

        int rc = mdb_txn_renew(instrument->dbtxn_read);
        if (rc == 0 && mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data) == 0) {
            memcpy((char *)shm_base + offset, &i, sizeof(int));
            offset += sizeof(int);
            size_t data_size = data.mv_size;
            memcpy((char *)shm_base + offset, &data_size, sizeof(size_t));
            offset += sizeof(size_t);
            memcpy((char *)shm_base + offset, data.mv_data, data_size);
            offset += data_size;
        }
        mdb_txn_reset(instrument->dbtxn_read);
    }

    // Unlock the semaphore
    sem_post(sem);

    // Clean up
    munmap(shm_base, shm_size);
    close(shmfd);
    sem_close(sem);

    return 0;
}

int astrid_instrument_restore_param_session_snapshot(lpinstrument_t * instrument, int snapshot_id) {
    int shmfd;
    sem_t * sem;
    void * shm_base;
    size_t offset = 0;
    char path[PATH_MAX] = {0};

    // Generate the path for the shared memory and semaphore
    snprintf(path, PATH_MAX, "%s-%s-%d", ASTRID_SESSION_SNAPSHOT_NAME, instrument->name, snapshot_id);

    // Open the semaphore
    if((sem = sem_open(path, 0)) == SEM_FAILED) {
        syslog(LOG_ERR, "Could not open semaphore. (%s) %s\n", path, strerror(errno));
        return -1;
    }

    // Open the shared memory segment
    if((shmfd = shm_open(path, O_RDWR, LPIPC_PERMS)) < 0) {
        syslog(LOG_ERR, "Could not open shared memory segment. (%s) %s\n", path, strerror(errno));
        sem_close(sem);
        return -1;
    }

    // Get the size of the shared memory segment
    struct stat shm_stat;
    if(fstat(shmfd, &shm_stat) < 0) {
        syslog(LOG_ERR, "Could not get shared memory segment size. (%s) %s\n", path, strerror(errno));
        close(shmfd);
        sem_close(sem);
        return -1;
    }
    size_t shm_size = shm_stat.st_size;

    if((shm_base = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
        syslog(LOG_ERR, "Could not mmap shared memory segment. (%s) %s\n", path, strerror(errno));
        close(shmfd);
        sem_close(sem);
        return -1;
    }

    // Lock the semaphore
    sem_wait(sem);

    // Read data from shared memory
    while(offset < shm_size) {
        int param_index;
        size_t data_size;
        void *data;

        memcpy(&param_index, (char *)shm_base + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&data_size, (char *)shm_base + offset, sizeof(size_t));
        offset += sizeof(size_t);

        data = malloc(data_size);
        if(!data) {
            syslog(LOG_ERR, "Failed to allocate memory for parameter data.");
            sem_post(sem);
            munmap(shm_base, shm_size);
            close(shmfd);
            sem_close(sem);
            return -1;
        }

        memcpy(data, (char *)shm_base + offset, data_size);
        offset += data_size;

        MDB_val key, mdb_data;
        key.mv_size = sizeof(int);
        key.mv_data = &param_index;
        mdb_data.mv_size = data_size;
        mdb_data.mv_data = data;

        int rc = mdb_txn_begin(instrument->dbenv, NULL, 0, &instrument->dbtxn_write);
        if(rc) {
            syslog(LOG_ERR, "Failed to begin transaction: (%d) %s", rc, mdb_strerror(rc));
            free(data);
            sem_post(sem);
            munmap(shm_base, shm_size);
            close(shmfd);
            sem_close(sem);
            return -1;
        }

        rc = mdb_put(instrument->dbtxn_write, instrument->dbi, &key, &mdb_data, 0);
        if(rc) {
            syslog(LOG_ERR, "Failed to put parameter data: (%d) %s", rc, mdb_strerror(rc));
            free(data);
            mdb_txn_abort(instrument->dbtxn_write);
            sem_post(sem);
            munmap(shm_base, shm_size);
            close(shmfd);
            sem_close(sem);
            return -1;
        }

        rc = mdb_txn_commit(instrument->dbtxn_write);
        if(rc) {
            syslog(LOG_ERR, "Failed to commit transaction: (%d) %s", rc, mdb_strerror(rc));
            free(data);
            sem_post(sem);
            munmap(shm_base, shm_size);
            close(shmfd);
            sem_close(sem);
            return -1;
        }

        free(data);
    }

    // Unlock the semaphore
    sem_post(sem);

    // Clean up
    munmap(shm_base, shm_size);
    close(shmfd);
    sem_close(sem);

    return 0;
}


int32_t astrid_instrument_get_param_int32(lpinstrument_t * instrument, int param_index, int32_t default_value) {
    int rc, param_exists=0;
    MDB_val key, data;
    int32_t param = default_value;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(int32_t);

	rc = mdb_txn_renew(instrument->dbtxn_read);
    rc = mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data);
    if(rc == 0) {
        param = *((int32_t *)data.mv_data);
        param_exists = 1;
    }
    mdb_txn_reset(instrument->dbtxn_read);

    if(!param_exists) astrid_instrument_set_param_int32(instrument, param_index, param);

    return param;
}

void astrid_instrument_set_param_int32(lpinstrument_t * instrument, int param_index, int32_t value) {
    int rc;
	MDB_val key, data;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(int32_t);
    data.mv_data = (void *)(&value);

	rc = mdb_txn_begin(instrument->dbenv, NULL, 0, &instrument->dbtxn_write);
    rc = mdb_put(instrument->dbtxn_write, instrument->dbi, &key, &data, 0);
    rc = mdb_txn_commit(instrument->dbtxn_write);
    if(rc) {
        syslog(LOG_WARNING, "astrid_instrument_get_param_int32 mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
    }
}


lpfloat_t astrid_instrument_get_param_float(lpinstrument_t * instrument, int param_index, lpfloat_t default_value) {
    int rc, param_exists=0;
    MDB_val key, data;
    lpfloat_t param = default_value;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lpfloat_t);

	rc = mdb_txn_renew(instrument->dbtxn_read);
    rc = mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data);
    if(rc == 0) {
        param = *((lpfloat_t *)data.mv_data);
        param_exists = 1;
    }
    mdb_txn_reset(instrument->dbtxn_read);

    if(!param_exists) astrid_instrument_set_param_float(instrument, param_index, param);

    return param;
}

void astrid_instrument_set_param_float(lpinstrument_t * instrument, int param_index, lpfloat_t value) {
    int rc;
	MDB_val key, data;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lpfloat_t);
    data.mv_data = (void *)(&value);

	rc = mdb_txn_begin(instrument->dbenv, NULL, 0, &instrument->dbtxn_write);
    rc = mdb_put(instrument->dbtxn_write, instrument->dbi, &key, &data, 0);
    rc = mdb_txn_commit(instrument->dbtxn_write);
    if(rc) {
        syslog(LOG_WARNING, "astrid_instrument_set_param_float mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
    }
}


void astrid_instrument_set_param_patternbuf(lpinstrument_t * instrument, int param_index, lppatternbuf_t * patternbuf) {
    int rc;
	MDB_val key, data;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lppatternbuf_t);
    data.mv_data = (void *)patternbuf;

	rc = mdb_txn_begin(instrument->dbenv, NULL, 0, &instrument->dbtxn_write);
    rc = mdb_put(instrument->dbtxn_write, instrument->dbi, &key, &data, 0);
    rc = mdb_txn_commit(instrument->dbtxn_write);
    if(rc) {
        syslog(LOG_WARNING, "astrid_instrument_set_param_patternbuf mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
    }
}

lppatternbuf_t astrid_instrument_get_param_patternbuf(lpinstrument_t * instrument, int param_index) {
    int rc, param_exists=0;
    MDB_val key, data;
    lppatternbuf_t patternbuf = {1,{1}};

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lppatternbuf_t);

	rc = mdb_txn_renew(instrument->dbtxn_read);
    rc = mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data);
    if(rc == 0) {
        patternbuf = *((lppatternbuf_t *)data.mv_data);
        param_exists = 1;
    }
    mdb_txn_reset(instrument->dbtxn_read);

    if(!param_exists) astrid_instrument_set_param_patternbuf(instrument, param_index, &patternbuf);

    return patternbuf;
}


void astrid_instrument_set_param_float_list(lpinstrument_t * instrument, int param_index, lpfloat_t * value, size_t size) {
    int rc;
	MDB_val key, data;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lpfloat_t) * size;
    data.mv_data = (void *)value;

	rc = mdb_txn_begin(instrument->dbenv, NULL, 0, &instrument->dbtxn_write);
    rc = mdb_put(instrument->dbtxn_write, instrument->dbi, &key, &data, 0);
    rc = mdb_txn_commit(instrument->dbtxn_write);
    if(rc) {
        syslog(LOG_WARNING, "astrid_instrument_get_param_float_list mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
    }
}

void astrid_instrument_get_param_float_list(lpinstrument_t * instrument, int param_index, size_t size, lpfloat_t * list) {
    int rc, param_exists=0;
    MDB_val key, data;

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lpfloat_t) * size;

	rc = mdb_txn_renew(instrument->dbtxn_read);
    rc = mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data);
    if(rc == 0) {
        memcpy(list, data.mv_data, data.mv_size);
        param_exists = 1;
    }

    if(!param_exists) astrid_instrument_set_param_float_list(instrument, param_index, list, size);

    mdb_txn_reset(instrument->dbtxn_read);
}

lpfloat_t astrid_instrument_get_param_float_list_item(
    lpinstrument_t * instrument, 
    int param_index, 
    size_t size, 
    int item_index, 
    lpfloat_t default_value
) {
    int rc;
    MDB_val key, data;
    lpfloat_t * param_list = NULL;
    lpfloat_t param = default_value;

    assert((size_t)item_index < size);

    key.mv_size = sizeof(int);
    key.mv_data = (void *)(&param_index);
    data.mv_size = sizeof(lpfloat_t) * size;

    rc = mdb_txn_renew(instrument->dbtxn_read);
    rc = mdb_get(instrument->dbtxn_read, instrument->dbi, &key, &data);
    if(rc == 0) {
        param_list = (lpfloat_t *)data.mv_data;
    }

    if(param_list != NULL) {
        param = param_list[item_index];
    }

    mdb_txn_reset(instrument->dbtxn_read);

    return param;
}

int lpencode_with_prefix(char * prefix, size_t val, char * encoded) {
    size_t size = 0;
    unsigned char * bytes = (unsigned char *)(&val);
    size = snprintf(NULL, size,  "%s-%d-%d-%d-%d-%d-%d-%d-%d\n", prefix,
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], 
        bytes[6], bytes[7]);

    if(!size) {
        syslog(LOG_ERR, "Could not estimate space for key. (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    assert(size+1 < LPKEY_MAXLENGTH);

    if(snprintf(encoded, size, "%s-%d-%d-%d-%d-%d-%d-%d-%d\n", prefix,
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], 
        bytes[6], bytes[7]) < 0) {
        syslog(LOG_ERR, "Could not encode key. (%d) %s\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

size_t lpdecode_with_prefix(char * encoded) {
    char *token, *save=NULL;
    char copy[LPKEY_MAXLENGTH] = {};
    unsigned char bytes[sizeof(size_t)] = {};

    // strtok clobbers input
    memcpy(copy, encoded, sizeof(copy));

    token = strtok_r(copy, "-", &save);

    int i = 0;
    while((token = strtok_r(NULL, "-", &save)) != NULL) {
        bytes[i] = atoi(token); i++;
    }

    return *((size_t *)bytes);
}

#ifndef NOPYTHON
#if 0
int astrid_instrument_renderer_python_start(lpinstrument_t * instrument, char * python_script_path) {
    PyObject * pmodule;
    PyConfig config;
    PyStatus status;

    syslog(LOG_INFO, "Starting python renderer...\n");
    /* Prepare cyrenderer module for import */
    if(PyImport_AppendInittab("cyrenderer", PyInit_cyrenderer) == -1) {
        syslog(LOG_ERR, "Error: could not extend in-built modules table for renderer\n");
        return -1;
    }

    /* Init python interpreter */
    PyConfig_InitPythonConfig(&config);

    status = PyConfig_SetString(&config, &config.program_name, L"python3");
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        return -1;
    }

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        return -1;
    }
    /* Import cyrenderer */
    pmodule = PyImport_ImportModule("cyrenderer");
    if(!pmodule) {
        PyErr_Print();
        Py_Finalize();
        syslog(LOG_ERR, "Error: could not import cython renderer module\n");
        return -1;
    }
    /* Import python instrument module */
    if(astrid_load_instrument(python_script_path) < 0) {
        PyErr_Print();
        Py_Finalize();
        syslog(LOG_ERR, "Error while attempting to load astrid instrument\n");
        return -1;
    }

    instrument->schedule_python_render = astrid_schedule_python_render;

    syslog(LOG_INFO, "Astrid python renderer is now rendering!\n");

    return 0;
}

int astrid_instrument_renderer_python_stop() {
    syslog(LOG_INFO, "Astrid python renderer shutting down...\n");
    Py_Finalize();
    return 0;
}
#endif
#endif
