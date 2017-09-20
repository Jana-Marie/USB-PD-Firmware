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

#include "int_n.h"

#include <ch.h>
#include <hal.h>

#include <pdb.h>
#include "priorities.h"
#include "fusb302b.h"
#include "protocol_rx.h"
#include "protocol_tx.h"
#include "hard_reset.h"
#include "policy_engine.h"


/*
 * INT_N polling thread
 */
static THD_FUNCTION(IntNPoll, vcfg) {
    struct pdb_config *cfg = vcfg;

    union fusb_status status;
    eventmask_t events;

    while (true) {
        /* If the INT_N line is low */
        if (palReadLine(LINE_INT_N) == PAL_LOW) {
            /* Read the FUSB302B status and interrupt registers */
            fusb_get_status(&status);

            /* If the I_GCRCSENT flag is set, tell the Protocol RX thread */
            if (status.interruptb & FUSB_INTERRUPTB_I_GCRCSENT) {
                chEvtSignal(cfg->prl.rx_thread, PDB_EVT_PRLRX_I_GCRCSENT);
            }

            /* If the I_TXSENT or I_RETRYFAIL flag is set, tell the Protocol TX
             * thread */
            events = 0;
            if (status.interrupta & FUSB_INTERRUPTA_I_RETRYFAIL) {
                events |= PDB_EVT_PRLTX_I_RETRYFAIL;
            }
            if (status.interrupta & FUSB_INTERRUPTA_I_TXSENT) {
                events |= PDB_EVT_PRLTX_I_TXSENT;
            }
            chEvtSignal(pdb_prltx_thread, events);

            /* If the I_HARDRST or I_HARDSENT flag is set, tell the Hard Reset
             * thread */
            events = 0;
            if (status.interrupta & FUSB_INTERRUPTA_I_HARDRST) {
                events |= PDB_EVT_HARDRST_I_HARDRST;
            }
            if (status.interrupta & FUSB_INTERRUPTA_I_HARDSENT) {
                events |= PDB_EVT_HARDRST_I_HARDSENT;
            }
            chEvtSignal(cfg->prl.hardrst_thread, events);

            /* If the I_OCP_TEMP and OVRTEMP flags are set, tell the Policy
             * Engine thread */
            if (status.interrupta & FUSB_INTERRUPTA_I_OCP_TEMP
                    && status.status1 & FUSB_STATUS1_OVRTEMP) {
                chEvtSignal(pdb_pe_thread, PDB_EVT_PE_I_OVRTEMP);
            }

        }
        chThdSleepMilliseconds(1);
    }
}

void pdb_int_n_run(struct pdb_config *cfg)
{
    cfg->int_n.thread = chThdCreateStatic(cfg->int_n._wa,
            sizeof(cfg->int_n._wa), PDB_PRIO_PRL_INT_N, IntNPoll, cfg);
}
