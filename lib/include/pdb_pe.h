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

#ifndef PDB_PE_H
#define PDB_PE_H

#include <stdbool.h>
#include <stdint.h>

#include <ch.h>

#include "pdb_conf.h"

/*
 * Events for the Policy Engine thread, sent by user code
 */
/* Tell the PE to send a Get_Source_Cap message */
#define PDB_EVT_PE_GET_SOURCE_CAP EVENT_MASK(7)
/* Tell the PE that new power is required */
#define PDB_EVT_PE_NEW_POWER EVENT_MASK(8)


/*
 * Structure for Policy Engine thread and variables
 */
struct pdb_pe {
    /* Policy Engine thread and working area */
    THD_WORKING_AREA(_wa, PDB_PE_WA_SIZE);
    thread_t *thread;

    /* PE mailbox for received PD messages */
    mailbox_t mailbox;
    /* PD message header template */
    uint16_t hdr_template;

    /* The received message we're currently working with */
    union pd_msg *_message;
    /* The most recent Request from the DPM */
    union pd_msg *_last_dpm_request;
    /* Whether or not we have an explicit contract */
    bool _explicit_contract;
    /* Whether or not we're receiving minimum power */
    bool _min_power;
    /* The number of hard resets we've sent */
    int8_t _hard_reset_counter;
    /* The result of the last Type-C Current match comparison */
    int8_t _old_tcc_match;
    /* The index of the first PPS APDO */
    uint8_t _pps_index;
    /* Virtual timer for SinkPPSPeriodicTimer */
    virtual_timer_t _sink_pps_periodic_timer;
    /* Queue for the PE mailbox */
    msg_t _mailbox_queue[PDB_MSG_POOL_SIZE];
};


#endif /* PDB_PE_H */
