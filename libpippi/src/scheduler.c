#include "scheduler.h"

lpscheduler_t * scheduler_create(int);
lpfloat_t scheduler_read_channel(lpscheduler_t * s, int channel);
void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay, void (*done_callback)(void *), void * done_ctx);
void scheduler_destroy(lpscheduler_t * s);
void start_playing(lpscheduler_t * s, lpevent_t * e);
void scheduler_debug(lpscheduler_t * s);
void scheduler_cleanup_nursery(lpscheduler_t * s);

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
void start_waiting(lpscheduler_t * s, lpevent_t * e) {
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

void start_playing(lpscheduler_t * s, lpevent_t * e) {
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

void stop_playing(lpscheduler_t * s, lpevent_t * e) {
    lpevent_t * current;
    lpevent_t * prev;

    /* Execute done callback if one has been registered */
    if(e->done != NULL) e->done(e->ctx);

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

lpscheduler_t * scheduler_create(int channels) {
    lpscheduler_t * s;

    s = (lpscheduler_t *)LPMemoryPool.alloc(1, sizeof(lpscheduler_t));

    s->waiting_queue_head = NULL;
    s->playing_stack_head = NULL;
    s->nursery_head = NULL;

    s->channels = channels;
    s->now = 0;
    s->current_frame = (lpfloat_t *)LPMemoryPool.alloc(channels, sizeof(lpfloat_t));

    s->event_count = 0;

    return s;
}

/* look for events waiting to be scheduled */
void scheduler_update(lpscheduler_t * s) {
    lpevent_t * current;
    void * next;
    if(s->waiting_queue_head != NULL) {
        current = s->waiting_queue_head;
        while(current->next != NULL) {
            next = current->next;
            if(current->onset <= s->now) {
                start_playing(s, current);
            }
            current = (lpevent_t *)next;
        }
        if(current->onset <= s->now) {
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


void scheduler_mix_buffers(lpscheduler_t * s) {
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

void scheduler_advance_buffers(lpscheduler_t * s) {
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
    /* Move buffers to proper lists */
    scheduler_update(s);

    /* Mix currently playing buffers into s->current_frame */
    scheduler_mix_buffers(s);

    /* Advance the position for all playing buffers */
    scheduler_advance_buffers(s);

    /* Advance now */
    s->now += 1;
}

void scheduler_schedule_event(lpscheduler_t * s, 
        lpbuffer_t * buf, 
        size_t delay, 
        void (*done_callback)(void*), 
        void * done_ctx
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
    }

    e->buf = buf;
    e->pos = 0;
    e->onset = s->now + delay;
    e->done = done_callback;
    e->ctx = done_ctx;

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

const lpscheduler_factory_t LPScheduler = { scheduler_create, lpscheduler_tick, scheduler_is_playing, scheduler_count_waiting, scheduler_count_playing, scheduler_count_done, scheduler_schedule_event, scheduler_cleanup_nursery, scheduler_destroy };
