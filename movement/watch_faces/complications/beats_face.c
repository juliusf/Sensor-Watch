#include <stdlib.h>
#include <string.h>
#include "beats_face.h"
#include "watch.h"

const uint8_t BEAT_REFRESH_FREQUENCY = 8;

void beats_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    (void) context_ptr;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(beats_face_state_t));
    }
}

void beats_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    beats_face_state_t *state = (beats_face_state_t *)context;
    state->next_subsecond_update = 0;
    state->last_centibeat_displayed = 0;
    movement_request_tick_frequency(BEAT_REFRESH_FREQUENCY);
}

bool beats_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    beats_face_state_t *state = (beats_face_state_t *)context;
    if (event.event_type == EVENT_TICK && event.subsecond != state->next_subsecond_update) {
         return true; // math is hard, don't do it if we don't have to.
    }

    char buf[16];
    uint32_t centibeats;

    watch_date_time date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            date_time = watch_rtc_get_date_time();
            centibeats = clock2beats(date_time.unit.hour, date_time.unit.minute, date_time.unit.second, event.subsecond, movement_timezone_offsets[settings->bit.time_zone]);
            if (centibeats == state->last_centibeat_displayed) {
                // we missed this update, try again next subsecond
                state->next_subsecond_update = (event.subsecond + 1) % BEAT_REFRESH_FREQUENCY;
            } else {
                state->next_subsecond_update = (event.subsecond + 1 + (BEAT_REFRESH_FREQUENCY * 2 / 3)) % BEAT_REFRESH_FREQUENCY;
                state->last_centibeat_displayed = centibeats;
            }
            sprintf(buf, "bt  %6ld", centibeats);

            watch_display_string(buf, 0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_tick_animation_is_running()) watch_start_tick_animation(432);
            date_time = watch_rtc_get_date_time();
            centibeats = clock2beats(date_time.unit.hour, date_time.unit.minute, date_time.unit.second, event.subsecond, movement_timezone_offsets[settings->bit.time_zone]);
            sprintf(buf, "bt  %4ld  ", centibeats / 100);

            watch_display_string(buf, 0);
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;
        case EVENT_ALARM_BUTTON_DOWN:
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_ALARM_LONG_PRESS:
        default:
            break;
    }

    return true;
}

void beats_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    movement_request_tick_frequency(1);
}

uint32_t clock2beats(uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t subseconds, int16_t utc_offset) {
    uint32_t retval = seconds * 1000 + (subseconds * 1000) / (BEAT_REFRESH_FREQUENCY);
    retval += 60 * minutes * 1000;
    retval += hours * 60 * 60 * 1000;
    retval -= (utc_offset - 60) * 60 * 1000;

    retval /= 864; // convert to centibeats
    retval %= 100000;

    return retval;
}