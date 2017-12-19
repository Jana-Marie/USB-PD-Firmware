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
 * PD Buddy firmware library Device Policy Manager callbacks
 *
 * All functions are passed a struct pdb_config * as their first parameter.
 * This points to the struct pdb_config that contains the calling thread.
 *
 * Optional functions may be set to NULL if the associated functionality is not
 * required.
 */
struct pdb_dpm_callbacks {
    /*
     * Evaluate the Source_Capabilities, creating a Request in response.
     *
     * The second parameter is the Source_Capabilities message.  This is NULL
     * when the function is called as a result of the PDB_EVT_PE_NEW_POWER
     * event.
     *
     * The third parameter is a union pd_msg * into which the Request must be
     * written.
     *
     * Returns true if sufficient power is available, false otherwise.
     */
    pdb_dpm_eval_cap_func evaluate_capability;

    /*
     * Create a Sink_Capabilities message for our current capabilities.
     *
     * The second parameter is a union pd_msg * into which the
     * Sink_Capabilities message must be written.
     */
    pdb_dpm_get_sink_cap_func get_sink_capability;

    /*
     * Return whether or not GiveBack support is enabled.
     *
     * Optional.  If the implementation does not support GiveBack under any
     * circumstances, this may be omitted.
     */
    pdb_dpm_giveback_func giveback_enabled;

    /*
     * Evaluate whether or not the Type-C Current can fulfill our power needs.
     *
     * The second parameter is an enum fusb_typec_current holding the Type-C
     * Current level to evaluate.
     *
     * Returns true if sufficient power is available, false otherwise.
     *
     * Optional.  If the implementation does not require Type-C Current support
     * (e.g. more than 5 V is required for the device to function), this may be
     * omitted.
     */
    pdb_dpm_tcc_func evaluate_typec_current;

    /*
     * Called at the start of Power Delivery negotiations.
     *
     * Optional.  If nothing special needs to happen when PD negotiations
     * start, this may be omitted.
     */
    pdb_dpm_func pd_start;

    /*
     * Transition the sink to the default power level for USB.
     */
    pdb_dpm_func transition_default;

    /*
     * Transition to the requested minimum current.
     *
     * Optional.  If and only if giveback_enabled is NULL, this may be omitted.
     */
    pdb_dpm_func transition_min;

    /*
     * Transition to Sink Standby if necessary.
     *
     * From section 7.2.3 of the USB PD spec, the sink shall reduce its
     * power draw to no more than 2.5 W before a positive or negative
     * transition of Vbus.  This function must determine if a voltage
     * transition is occurring, and if it is, it must reduce the power
     * consumption to the required level.
     */
    pdb_dpm_func transition_standby;

    /*
     * Transition to the requested power level.
     */
    pdb_dpm_func transition_requested;

    /*
     * Transition to the appropriate power level for the most recent Type-C
     * Current evaluated.
     *
     * Optional.  If and only if evaluate_typec_current is NULL, this may be
     * omitted.
     */
    pdb_dpm_func transition_typec;

    /*
     * Handle a received Not_Supported message.
     *
     * Optional.  If no special handling is needed, this may be omitted.
     */
    pdb_dpm_func not_supported_received;
};


#endif /* PDB_DPM_H */
