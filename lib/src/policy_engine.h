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

#ifndef PDB_POLICY_ENGINE_H
#define PDB_POLICY_ENGINE_H

#include <ch.h>

#include <pdb.h>

/*
 * Events for the Policy Engine thread, used internally
 *
 * NOTE: If any more events are needed, make sure they don't overlap the ones
 * in include/pdb_pe.h!
 */
#define PDB_EVT_PE_RESET EVENT_MASK(0)
#define PDB_EVT_PE_MSG_RX EVENT_MASK(1)
#define PDB_EVT_PE_TX_DONE EVENT_MASK(2)
#define PDB_EVT_PE_TX_ERR EVENT_MASK(3)
#define PDB_EVT_PE_HARD_SENT EVENT_MASK(4)
#define PDB_EVT_PE_I_OVRTEMP EVENT_MASK(5)
#define PDB_EVT_PE_PPS_REQUEST EVENT_MASK(6)


/*
 * Start the Policy Engine thread
 */
void pdb_pe_run(struct pdb_config *cfg);


#endif /* PDB_POLICY_ENGINE_H */
