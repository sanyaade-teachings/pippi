#include <jack/jack.h>
#include "astrid.h"


jack_port_t * astrid_inport1, * astrid_inport2;
jack_client_t * jack_client;
int adc_shmid;

/* When false, the inner loop exits and begins cleanup */
static volatile int adc_is_running = 1;

/* When false, no frames are written into the shared buffer */
static volatile int adc_is_capturing = 1;

void handle_shutdown(__attribute__((unused)) int sig) {
    adc_is_capturing = 0;
    adc_is_running = 0;
}

/* Audio callback: writes a block of frames into 
 * the shared circular buffer and increments the 
 * write position in the buffer. */
int audio_callback(jack_nframes_t nframes, __attribute__((unused)) void * arg) {
    lpipc_buffer_t * adcbuf;
    jack_nframes_t i;
    jack_default_audio_sample_t * in1, * in2;
    size_t idx;

    if(adc_is_capturing == 0) return 0;

    if(lpadc_aquire(&adcbuf) < 0) {
        syslog(LOG_ERR, "Could not aquire ADC shared memory in JACK callback");
        return -1;
    }

    in1 = jack_port_get_buffer(astrid_inport1, nframes);
    in2 = jack_port_get_buffer(astrid_inport2, nframes);

    /* Copy the block of samples */
    for(i=0; i < nframes; i++) {
        idx = (adcbuf->pos + i * ASTRID_CHANNELS) % LPADCBUFSAMPLES;
        adcbuf->data[idx] = (lpfloat_t)(*in1++);
        adcbuf->data[(idx + 1) % LPADCBUFSAMPLES] = (lpfloat_t)(*in2++);
    }

    if(lpadc_increment_and_release(adcbuf, nframes * ASTRID_CHANNELS) < 0) {
        syslog(LOG_ERR, "Could not write input block to ADC in JACK callback");
        return -1;
    }

    return 0;
}

int main() {
    jack_status_t jack_status;
    jack_options_t jack_options = JackNullOption;

    /* Open a handle to the system log */
    openlog("astrid-adc", LOG_PID, LOG_USER);

    /* setup signal handlers */
    struct sigaction shutdown_action;
    shutdown_action.sa_handler = handle_shutdown;
    sigemptyset(&shutdown_action.sa_mask);
    shutdown_action.sa_flags = SA_RESTART; /* Prevent open, read, write etc from EINTR */

    /* Keyboard interrupt triggers cleanup and shutdown */
    if(sigaction(SIGINT, &shutdown_action, NULL) == -1) {
        perror("Could not init SIGINT signal handler");
        exit(1);
    }

    /* Terminate signals also trigger cleanup and shutdown */
    if(sigaction(SIGTERM, &shutdown_action, NULL) == -1) {
        perror("Could not init SIGTERM signal handler");
        exit(1);
    }
    
    /* Create a shared memory region to be used as a circular 
     * buffer for renderer processes to read from */
    syslog(LOG_DEBUG, "Creating shared memory buffer for ADC\n");
    if((adc_shmid = lpadc_create()) < 0) {
        perror("Could not create adcbuf shared memory");
        return 1;
    }

    syslog(LOG_INFO, "Created ADC shared memory with shmid %d\n", adc_shmid);

    /* Set up JACK */
    syslog(LOG_DEBUG, "Starting jack...\n");
    jack_client = jack_client_open("astrid-adc", jack_options, &jack_status, NULL);
    syslog(LOG_DEBUG, "Got client pointer... printing status\n");
    syslog(LOG_DEBUG, "JACK STATUS %d\n", (int)jack_status);

    if(jack_client == NULL) {
        syslog(LOG_ERR, "Could not open jack client. Client is NULL: %s\n", strerror(errno));

        if((jack_status & JackServerFailed) == JackServerFailed) {
            syslog(LOG_ERR, "Could not open jack client. Jack server failed with status %2.0x\n", jack_status);
        } else {
            syslog(LOG_ERR, "Could not open jack client. Unknown error: %s\n", strerror(errno));
        }
        goto exit_with_error;
    }

    if((jack_status & JackServerStarted) == JackServerStarted) {
        syslog(LOG_INFO, "Jack server started!\n");
    }

    syslog(LOG_DEBUG, "Setting process callback...\n");
    jack_set_process_callback(jack_client, audio_callback, NULL);

    syslog(LOG_DEBUG, "Registering inport1...\n");
    astrid_inport1 = jack_port_register(jack_client, "inL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    syslog(LOG_DEBUG, "Registering inport2...\n");
    astrid_inport2 = jack_port_register(jack_client, "inR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    if((astrid_inport1 == NULL) || (astrid_inport2 == NULL)) {
        syslog(LOG_ERR, "No more JACK ports available, shutting down...\n");
        goto exit_with_error;
    }

    syslog(LOG_DEBUG, "Activating the client...\n");
    if(jack_activate(jack_client) != 0) {
        syslog(LOG_ERR, "Could not activate JACK client, shutting down...\n");
        goto exit_with_error;
    }

    /* Wait for JACK to settle */
    usleep((useconds_t)1000000);

    syslog(LOG_DEBUG, "ADC is now running!\n");
    while(adc_is_running) {
        usleep((useconds_t)1000);
    }

    syslog(LOG_DEBUG, "Exiting normally: shutting down audio\n");
    jack_client_close(jack_client);

    syslog(LOG_DEBUG, "Exiting normally: cleaning up shared buffer\n");
    lpadc_destroy();
    return 0;

exit_with_error:
    syslog(LOG_DEBUG, "Attempting to clean up after exiting with error\n");
    jack_client_close(jack_client);
    lpadc_destroy();
    return 1;
}

