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

#ifndef PDB_DPM_H
#define PDB_DPM_H

#include <stdbool.h>

#include <pdb_msg.h>
/* TODO: improve this include */
#include "fusb302b.h"


/* Forward declaration of struct pdb_config */
struct pdb_config;

/* DPM callback typedefs */
typedef void (*pdb_dpm_func)(struct pdb_config *);
typedef bool (*pdb_dpm_eval_cap_func)(struct pdb_config *,
        const union pd_msg *, union pd_msg *);
typedef void (*pdb_dpm_get_sink_cap_func)(struct pdb_config *, union pd_msg *);
typedef bool (*pdb_dpm_giveback_func)(struct pdb_config *);
typedef bool (*pdb_dpm_tcc_func)(struct pdb_config *, enum fusb_typec_current);

/*
 * PD Buddy firmware library device policy manager callback structure
 *
 * Optional functions may be set to NULL if the associated functionality is not
 * required.
 */
struct pdb_dpm_callbacks {
    pdb_dpm_eval_cap_func evaluate_capability;
    pdb_dpm_get_sink_cap_func get_sink_capability;
    pdb_dpm_giveback_func giveback_enabled;
    pdb_dpm_tcc_func evaluate_typec_current; /* Optional */
    pdb_dpm_func pd_start; /* Optional */
    /* dpm_sink_standby is called in PE_SNK_Select_Capability to ensure power
     * consumption is less than 2.5 W.
     *   - dpm_transition_standby()
     */
    /* dpm_output_set is called in five places:
     *   - PS_RDY received → transition sink power supply to our request
     *     - This could mean what we wanted, or what we had to settle for, so
     *       we have to remember.
     *     - dpm_transition_requested()
     *   - Protocol error in PE_SNK_Transition_Sink
     *     - Unnecessary?  If we really want to be defensive here, just go to
     *       default output, right?  That's what we'll do shortly afterwards
     *       anyway.
     *     - dpm_transition_default()
     *   - GotoMin received and giveback → transition sink to min power
     *     - dpm_transition_min()
     *   - Transition sink power supply to match Type-C Current advertisement
     *     - dpm_transition_typec() (if I don't like it I can change it)
     *   - Part of dpm_output_default
     *     - Obsolete (see below)
     */
    /* dpm_output_default is only called in PE_SNK_Transition_to_Default.
     * Should probably be part of new improved replacement for dpm_output_set.
     *   - dpm_transition_default()
     */
    pdb_dpm_func transition_default;
    pdb_dpm_func transition_min; /* Optional if no GiveBack */
    pdb_dpm_func transition_standby;
    pdb_dpm_func transition_requested;
    pdb_dpm_func transition_typec; /* Optional */
};


#endif /* PDB_DPM_H */
