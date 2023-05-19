#include "astrid.h"

lpscheduler_t * scheduler_create(int, int, lpfloat_t);
lpfloat_t scheduler_read_channel(lpscheduler_t * s, int channel);
void scheduler_schedule_event(lpscheduler_t * s, lpbuffer_t * buf, size_t delay, void (*callback)(lpmsg_t), lpmsg_t msg, size_t callback_delay);
void scheduler_destroy(lpscheduler_t * s);
static inline void start_playing(lpscheduler_t * s, lpevent_t * e);
void scheduler_debug(lpscheduler_t * s);
void scheduler_cleanup_nursery(lpscheduler_t * s);
void scheduler_frames_to_timespec(size_t frames, lpfloat_t samplerate, struct timespec * ts);
void lpscheduler_tick(lpscheduler_t * s);
void lpscheduler_handle_callbacks(lpscheduler_t * s);


