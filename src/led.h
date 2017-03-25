/*
 * PD Buddy - USB Power Delivery for everyone
 * Copyright (C) 2017 Clayton G. Hobbs <clay@lakeserv.net>
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

#ifndef PDB_LED_H
#define PDB_LED_H

#include <ch.h>


/* Events for the LED thread */
#define PDB_EVT_LED_OFF EVENT_MASK(0)
#define PDB_EVT_LED_ON EVENT_MASK(1)
#define PDB_EVT_LED_FAST_BLINK EVENT_MASK(2)
#define PDB_EVT_LED_MEDIUM_BLINK_OFF EVENT_MASK(3)
#define PDB_EVT_LED_SLOW_BLINK EVENT_MASK(4)
#define PDB_EVT_LED_FAST_BLINK_SLOW EVENT_MASK(5)

/* The LED thread object */
extern thread_t *pdb_led_thread;

/*
 * Start the LED thread
 */
void pdb_led_run(void);


#endif /* PDB_LED_H */
