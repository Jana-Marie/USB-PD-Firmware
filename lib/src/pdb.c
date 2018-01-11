/*
 * PD Buddy Firmware Library - USB Power Delivery for everyone
 * Copyright 2017-2018 Clayton G. Hobbs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pdb.h>
#include "policy_engine.h"
#include "protocol_rx.h"
#include "protocol_tx.h"
#include "hard_reset.h"
#include "int_n.h"
#include "fusb302b.h"
#include "messages.h"


void pdb_init(struct pdb_config *cfg)
{
    /* Initialize the empty message pool */
    pdb_msg_pool_init();

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
