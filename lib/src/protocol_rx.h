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

#ifndef PDB_PROTOCOL_RX_H
#define PDB_PROTOCOL_RX_H

#include <stdint.h>

#include <ch.h>

#include <pdb.h>


/* Events for the Protocol RX thread */
#define PDB_EVT_PRLRX_RESET EVENT_MASK(0)
#define PDB_EVT_PRLRX_I_GCRCSENT EVENT_MASK(1)

/* The Protocol RX thread object */
extern thread_t *pdb_prlrx_thread;

/* The last received MessageID */
extern int8_t pdb_prlrx_messageid;

/*
 * Start the Protocol RX thread
 */
void pdb_prlrx_run(struct pdb_config *cfg);


#endif /* PDB_PROTOCOL_RX_H */
