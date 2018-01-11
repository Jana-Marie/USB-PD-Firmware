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

#ifndef PDB_PROTOCOL_RX_H
#define PDB_PROTOCOL_RX_H

#include <stdint.h>

#include <ch.h>

#include <pdb.h>


/* Events for the Protocol RX thread */
#define PDB_EVT_PRLRX_RESET EVENT_MASK(0)
#define PDB_EVT_PRLRX_I_GCRCSENT EVENT_MASK(1)

/*
 * Start the Protocol RX thread
 */
void pdb_prlrx_run(struct pdb_config *cfg);


#endif /* PDB_PROTOCOL_RX_H */
