/*
 * PD Buddy - USB Power Delivery for everyone
 * Copyright (C) 2017-2018 Clayton G. Hobbs <clay@lakeserv.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "led.h"

#include <hal.h>

#include "priorities.h"

/* Delays for blink modes */
#define LED_FAST MS2ST(125)
#define LED_MEDIUM MS2ST(250)
#define LED_SLOW MS2ST(500)

/* Number of blinks for temporary modes */
#define LED_MEDIUM_BLINKS 3
#define LED_FAST_BLINKS 8


thread_t *pdbs_led_thread;

/*
 * LED blinker thread.
 */
static THD_WORKING_AREA(waLED, 128);
static THD_FUNCTION(LED, arg) {
    (void) arg;
    /* LED starts turned off */
    eventmask_t state = PDBS_EVT_LED_OFF;
    /* The event just received */
    eventmask_t newstate;
    /* Timeout.  TIME_INFINITE for steady modes, other values for blinking
     * modes. */
    systime_t timeout = TIME_INFINITE;
    /* Counter for blinking modes */
    int i = 0;

    while (true) {
        /* Wait for any event except the last one we saw */
        newstate = chEvtWaitOneTimeout(ALL_EVENTS & ~state, timeout);
        /* If we're changing state, reset the counter */
        if (newstate != 0) {
            /* Clear the events to make sure we don't get an unexpected state
             * change.  As an example of such an unexpected state change, if we
             * set the state to ON, then ON, then BLINK, the second ON wouldn't
             * be seen until after we handled the BLINK event, causing us to
             * end up in the ON state in the end. */
            chEvtGetAndClearEvents(ALL_EVENTS);
            state = newstate;
            i = 0;
        }

        switch (state) {
            case PDBS_EVT_LED_OFF:
                timeout = TIME_INFINITE;
                palClearLine(LINE_LED);
                break;
            case PDBS_EVT_LED_ON:
                timeout = TIME_INFINITE;
                palSetLine(LINE_LED);
                break;
            case PDBS_EVT_LED_FAST_BLINK:
                timeout = LED_FAST;
                if (i == 0) {
                    palSetLine(LINE_LED);
                } else {
                    palToggleLine(LINE_LED);
                }
                break;
            case PDBS_EVT_LED_MEDIUM_BLINK_OFF:
                timeout = LED_MEDIUM;
                if (i == 0) {
                    palSetLine(LINE_LED);
                } else if (i < (LED_MEDIUM_BLINKS * 2) - 1) {
                    palToggleLine(LINE_LED);
                } else {
                    palClearLine(LINE_LED);
                    timeout = TIME_INFINITE;
                }
                break;
            case PDBS_EVT_LED_SLOW_BLINK:
                timeout = LED_SLOW;
                if (i == 0) {
                    palSetLine(LINE_LED);
                } else {
                    palToggleLine(LINE_LED);
                }
                break;
            case PDBS_EVT_LED_FAST_BLINK_SLOW:
                timeout = LED_FAST;
                if (i == 0) {
                    palSetLine(LINE_LED);
                } else if (i < (LED_FAST_BLINKS * 2)) {
                    palToggleLine(LINE_LED);
                } else {
                    palClearLine(LINE_LED);
                    timeout = TIME_INFINITE;
                    chEvtSignal(pdbs_led_thread, PDBS_EVT_LED_SLOW_BLINK);
                }
                break;
            default:
                break;
        }

        /* Increment the counter for the blinking modes */
        i++;
    }
}

void pdbs_led_run(void)
{
    pdbs_led_thread = chThdCreateStatic(waLED, sizeof(waLED), PDBS_PRIO_LED,
            LED, NULL);
}
