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


struct pdb_pe {
    THD_WORKING_AREA(_wa, 128);
    thread_t *thread;

    mailbox_t mailbox;

    union pd_msg *_policy_engine_message;
    union pd_msg *_last_dpm_request;
    bool _capability_match;
    bool _explicit_contract;
    bool _min_power;
    int8_t _hard_reset_counter;
    msg_t _mailbox_queue[PDB_MSG_POOL_SIZE];
};


#endif /* PDB_PE_H */
