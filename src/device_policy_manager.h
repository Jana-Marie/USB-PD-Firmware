/*
 * PD Buddy Sink Firmware - Smart power jack for USB Power Delivery
 * Copyright (C) 2017-2018 Clayton G. Hobbs <clay@lakeserv.net>
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

#ifndef PDBS_DEVICE_POLICY_MANAGER_H
#define PDBS_DEVICE_POLICY_MANAGER_H

#include <stdbool.h>

#include <pdb.h>


struct pdbs_dpm_data {
    /* The most recently received Source_Capabilities message */
    const union pd_msg *capabilities;
    /* The most recently received Type-C Current advertisement */
    enum fusb_typec_current typec_current;
    /* Whether the DPM is able to turn on the output */
    bool output_enabled;
    /* Whether the DPM sets the LED to indicate the PD status */
    bool led_pd_status;
    /* Whether the device is capable of USB communications */
    bool usb_comms;

    /* Whether or not the power supply is unconstrained */
    bool _unconstrained_power;
    /* Whether our capabilities matched or not */
    bool _capability_match;
    /* The last explicitly or implicitly negotiated voltage, in millivolts */
    int _present_voltage;
    /* The requested voltage, in millivolts */
    int _requested_voltage;
};


/*
 * Create a Request message based on the given Source_Capabilities message.  If
 * capabilities is NULL, the last non-null Source_Capabilities message passes
 * is used.  If none has been provided, the behavior is undefined.
 *
 * Returns true if sufficient power is available, false otherwise.
 */
bool pdbs_dpm_evaluate_capability(struct pdb_config *cfg,
        const union pd_msg *capabilities, union pd_msg *request);

/*
 * Create a Sink_Capabilities message for our current capabilities.
 */
void pdbs_dpm_get_sink_capability(struct pdb_config *cfg, union pd_msg *cap);

/*
 * Return whether or not GiveBack support is enabled.
 */
bool pdbs_dpm_giveback_enabled(struct pdb_config *cfg);

/*
 * Evaluate whether or not the currently offered Type-C Current can fulfill our
 * power needs.
 *
 * Returns true if sufficient power is available, false otherwise.
 */
bool pdbs_dpm_evaluate_typec_current(struct pdb_config *cfg, enum fusb_typec_current tcc);

/*
 * Indicate that power negotiations are starting.
 */
void pdbs_dpm_pd_start(struct pdb_config *cfg);

/*
 * Transition the sink to default power.
 */
void pdbs_dpm_transition_default(struct pdb_config *cfg);

/*
 * Transition to the requested minimum current.
 */
void pdbs_dpm_transition_min(struct pdb_config *cfg);

/*
 * Transition to Sink Standby if necessary.
 */
void pdbs_dpm_transition_standby(struct pdb_config *cfg);

/*
 * Transition to the requested power level
 */
void pdbs_dpm_transition_requested(struct pdb_config *cfg);

/*
 * Transition to the Type-C Current power level
 */
void pdbs_dpm_transition_typec(struct pdb_config *cfg);


#endif /* PDBS_DEVICE_POLICY_MANAGER_H */
