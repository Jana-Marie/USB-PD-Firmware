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

#ifndef PDB_H
#define PDB_H

#include <pdb_fusb.h>
#include <pdb_dpm.h>
#include <pdb_pe.h>
#include <pdb_prl.h>
#include <pdb_int_n.h>
#include <pdb_msg.h>


/* Version information */
#define PDB_LIB_VERSION "0.1.0"
#define PDB_LIB_MAJOR 0
#define PDB_LIB_MINOR 1
#define PDB_LIB_PATCH 0


/*
 * Structure for one USB port's PD Buddy firmware library configuration
 *
 * Contains working areas for statically allocated threads, and therefore must
 * be statically allocated!
 */
struct pdb_config {
    /* User-initialized fields */
    /* Configuration information for the FUSB302B* chip */
    struct pdb_fusb_config fusb;
    /* DPM callbacks */
    struct pdb_dpm_callbacks dpm;
    /* Pointer to port-specific DPM data */
    void *dpm_data;

    /* Automatically initialized fields */
    /* Policy Engine thread and related variables */
    struct pdb_pe pe;
    /* Protocol layer threads and related variables */
    struct pdb_prl prl;
    /* INT_N pin thread and related variables */
    struct pdb_int_n int_n;
};


/*
 * Initialize the PD Buddy firmware library, starting all its threads
 *
 * The I2C driver must already be initialized before calling this function.
 */
void pdb_init(struct pdb_config *);


#endif /* PDB_H */
