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

#ifndef PDB_POLICY_ENGINE_H
#define PDB_POLICY_ENGINE_H

#include <ch.h>


/* Events for the Policy Engine thread */
#define PDB_EVT_PE_RESET EVENT_MASK(0)
#define PDB_EVT_PE_MSG_RX EVENT_MASK(1)
#define PDB_EVT_PE_TX_DONE EVENT_MASK(2)
#define PDB_EVT_PE_TX_ERR EVENT_MASK(3)
#define PDB_EVT_PE_HARD_SENT EVENT_MASK(4)
#define PDB_EVT_PE_I_OVRTEMP EVENT_MASK(5)

/* The Policy Engine thread object */
extern thread_t *pdb_pe_thread;

/* Policy Engine thread mailbox */
extern mailbox_t pdb_pe_mailbox;

/*
 * Start the Policy Engine thread
 */
void pdb_pe_run(void);


#endif /* PDB_POLICY_ENGINE_H */
