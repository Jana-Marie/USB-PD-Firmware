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

#ifndef PDB_HARD_RESET_H
#define PDB_HARD_RESET_H

#include <ch.h>

#include <pdb.h>


/* Events for the Hard Reset thread */
#define PDB_EVT_HARDRST_RESET EVENT_MASK(0)
#define PDB_EVT_HARDRST_I_HARDRST EVENT_MASK(1)
#define PDB_EVT_HARDRST_I_HARDSENT EVENT_MASK(2)
#define PDB_EVT_HARDRST_DONE EVENT_MASK(3)

/*
 * Start the Hard Reset thread
 */
void pdb_hardrst_run(struct pdb_config *cfg);


#endif /* PDB_HARD_RESET_H */
