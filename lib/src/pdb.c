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

#include <pdb.h>
#include "policy_engine.h"
#include "protocol_rx.h"
#include "protocol_tx.h"
#include "hard_reset.h"
#include "int_n.h"
#include "fusb302b.h"


void pdb_init(struct pdb_config *cfg)
{
    /* Initialize the FUSB302B */
    fusb_setup(&cfg->fusb);

    /* Create the policy engine thread. */
    pdb_pe_run(cfg);

    /* Create the protocol layer threads. */
    pdb_prlrx_run(cfg);
    pdb_prltx_run(cfg);
    pdb_hardrst_run(cfg);

    /* Create the INT_N thread. */
    pdb_int_n_run(cfg);
}
