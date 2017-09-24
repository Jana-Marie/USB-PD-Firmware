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

#include "pdb_conf.h"


/*
 * Structure for the protocol layer threads and variables
 */
struct pdb_prl {
    /* RX thread and working area */
    THD_WORKING_AREA(_rx_wa, PDB_PRLRX_WA_SIZE);
    thread_t *rx_thread;
    /* TX thread and working area */
    THD_WORKING_AREA(_tx_wa, PDB_PRLTX_WA_SIZE);
    thread_t *tx_thread;
    /* Hard reset thread and working area */
    THD_WORKING_AREA(_hardrst_wa, PDB_HARDRST_WA_SIZE);
    thread_t *hardrst_thread;

    /* TX mailbox for PD messages to be transmitted */
    mailbox_t tx_mailbox;

    /* The ID of the last message received */
    int8_t _rx_messageid;
    /* The message being worked with by the RX thread */
    union pd_msg *_rx_message;

    /* The ID of the next message we will transmit */
    int8_t _tx_messageidcounter;
    /* The message being worked with by the TX thread */
    union pd_msg *_tx_message;
    /* Queue for the TX mailbox */
    msg_t _tx_mailbox_queue[PDB_MSG_POOL_SIZE];
};


#endif /* PDB_PRL_H */
