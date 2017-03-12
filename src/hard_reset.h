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

#ifndef PDB_HARD_RESET_H
#define PDB_HARD_RESET_H

#include <ch.h>


/* Events for the Hard Reset thread */
#define PDB_EVT_HARDRST_RESET EVENT_MASK(0)
#define PDB_EVT_HARDRST_I_HARDRST EVENT_MASK(1)
#define PDB_EVT_HARDRST_I_HARDSENT EVENT_MASK(2)
#define PDB_EVT_HARDRST_DONE EVENT_MASK(3)

/* The Hard Reset thread object */
extern thread_t *pdb_hardrst_thread;

/*
 * Start the Hard Reset thread
 */
void pdb_hardrst_run(void);


#endif /* PDB_HARD_RESET_H */
