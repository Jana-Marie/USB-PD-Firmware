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

#include <pdb_fusb.h>
#include <pdb_msg.h>


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
 * required. (TODO)
 */
struct pdb_dpm_callbacks {
    pdb_dpm_eval_cap_func evaluate_capability;
    pdb_dpm_get_sink_cap_func get_sink_capability;
    pdb_dpm_giveback_func giveback_enabled;
    pdb_dpm_tcc_func evaluate_typec_current; /* Optional */
    pdb_dpm_func pd_start; /* Optional */
    pdb_dpm_func transition_default;
    pdb_dpm_func transition_min; /* Optional if no GiveBack */
    pdb_dpm_func transition_standby;
    pdb_dpm_func transition_requested;
    pdb_dpm_func transition_typec; /* Optional */
};


#endif /* PDB_DPM_H */
