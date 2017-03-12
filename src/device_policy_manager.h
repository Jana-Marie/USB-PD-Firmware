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

#ifndef PDB_DEVICE_POLICY_MANAGER_H
#define PDB_DEVICE_POLICY_MANAGER_H

#include <stdbool.h>

#include "messages.h"


/*
 * Create a Request message based on the given Source_Capabilities message.
 *
 * Returns false if sufficient power is not available, true if it is.
 */
bool pdb_dpm_evaluate_capability(const union pd_msg *capabilities, union pd_msg *request);

/*
 * Create a Sink_Capabilities message for our current capabilities.
 */
void pdb_dpm_get_sink_capability(union pd_msg *cap);

/*
 * Turn on the power output, with LED indication.
 */
void pdb_dpm_output_on(void);

/*
 * Turn off the power output, with LED indication.
 */
void pdb_dpm_output_off(void);


#endif /* PDB_DEVICE_POLICY_MANAGER_H */
