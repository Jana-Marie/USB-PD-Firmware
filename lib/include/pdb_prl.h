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

#ifndef PDB_PRL_H
#define PDB_PRL_H

#include <stdint.h>

#include <ch.h>


struct pdb_prl {
    THD_WORKING_AREA(_rx_wa, 256);
    thread_t *rx_thread;
    THD_WORKING_AREA(_tx_wa, 256);
    thread_t *tx_thread;
    THD_WORKING_AREA(_hardrst_wa, 256);
    thread_t *hardrst_thread;

    mailbox_t tx_mailbox;

    int8_t _rx_messageid;
    union pd_msg *_rx_message;

    int8_t _tx_messageidcounter;
    union pd_msg *_tx_message;
    msg_t _tx_mailbox_queue[PDB_MSG_POOL_SIZE];
};


#endif /* PDB_PRL_H */
