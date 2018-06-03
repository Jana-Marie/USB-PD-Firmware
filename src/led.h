/*
 * PD Buddy Sink Firmware - Smart power jack for USB Power Delivery
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

#ifndef PDBS_LED_H
#define PDBS_LED_H

#include <ch.h>


/* Events for the LED thread */
#define PDBS_EVT_LED_OFF EVENT_MASK(0)
#define PDBS_EVT_LED_ON EVENT_MASK(1)
#define PDBS_EVT_LED_FAST_BLINK EVENT_MASK(2)
#define PDBS_EVT_LED_MEDIUM_BLINK_OFF EVENT_MASK(3)
#define PDBS_EVT_LED_SLOW_BLINK EVENT_MASK(4)
#define PDBS_EVT_LED_FAST_BLINK_SLOW EVENT_MASK(5)
#define PDBS_EVT_LED_LONG_BLINK EVENT_MASK(6)

/* Semantic LED event names */
#define PDBS_EVT_LED_CONFIG PDBS_EVT_LED_SLOW_BLINK
#define PDBS_EVT_LED_IDENTIFY PDBS_EVT_LED_FAST_BLINK_SLOW
#define PDBS_EVT_LED_NEGOTIATING PDBS_EVT_LED_FAST_BLINK
#define PDBS_EVT_LED_OUTPUT_ON PDBS_EVT_LED_MEDIUM_BLINK_OFF
#define PDBS_EVT_LED_OUTPUT_OFF PDBS_EVT_LED_ON
#define PDBS_EVT_LED_OUTPUT_OFF_NO_TYPEC PDBS_EVT_LED_LONG_BLINK

/* The LED thread object */
extern thread_t *pdbs_led_thread;

/*
 * Start the LED thread
 */
void pdbs_led_run(void);


#endif /* PDBS_LED_H */
