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

#ifndef PDB_H
#define PDB_H

#include <pdb_fusb.h>
#include <pdb_dpm.h>
#include <pdb_pe.h>
#include <pdb_prl.h>
#include <pdb_int_n.h>


/*
 * Structure for PD Buddy firmware library configuration
 *
 * Contains working areas for statically allocated threads, and therefore must
 * be statically allocated!
 */
struct pdb_config {
    /* Configuration information for the FUSB302B* chip */
    struct pdb_fusb_config fusb_config;
    /* DPM callbacks */
    struct pdb_dpm_callbacks dpm;
    /* Pointer to port-specific DPM data */
    void *dpm_data;
    /* Policy Engine thread and related variables */
    struct pdb_pe pe;
    /* Protocol layer threads and related variables */
    struct pdb_prl prl;
    /* INT_N pin thread and related variables */
    struct pdb_int_n int_n;
};


/*
 * Initialize the PD Buddy firmware library, starting all its threads
 */
void pdb_init(struct pdb_config *);


#endif /* PDB_H */
