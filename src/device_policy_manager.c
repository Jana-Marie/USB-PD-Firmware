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

#include "device_policy_manager.h"

#include <stdint.h>

#include <hal.h>

#include "led.h"
#include "storage.h"
#include "pd.h"


/* Whether or not the power supply is unconstrained */
static uint8_t dpm_unconstrained_power;

bool pdb_dpm_evaluate_capability(const union pd_msg *capabilities, union pd_msg *request)
{
    /* Get the current configuration */
    struct pdb_config *cfg = pdb_config_flash_read();
    /* Get the number of PDOs */
    uint8_t numobj = PD_NUMOBJ_GET(capabilities);

    /* Make the LED blink to indicate ongoing power negotiations */
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_FAST_BLINK);

    /* Get whether or not the power supply is constrained */
    if (capabilities->obj[0] & PD_PDO_SRC_FIXED_UNCONSTRAINED) {
        dpm_unconstrained_power = 1;
    } else {
        dpm_unconstrained_power = 0;
    }

    /* Make sure we have configuration */
    if (cfg != NULL) {
        /* Look at the PDOs to see if one matches our desires */
        for (uint8_t i = 0; i < numobj; i++) {
            /* Fixed Supply PDOs come first, so when we see a PDO that isn't a
             * Fixed Supply, stop reading. */
            if ((capabilities->obj[i] & PD_PDO_TYPE) != PD_PDO_TYPE_FIXED) {
                break;
            }
            /* If the V from the PDO equals our desired V and the I is at least
             * our desired I */
            if (PD_PDO_SRC_FIXED_VOLTAGE_GET(capabilities, i) == cfg->v
                    && PD_PDO_SRC_FIXED_CURRENT_GET(capabilities, i) >= cfg->i) {
                /* We got what we wanted, so build a request for that */
                request->hdr = PD_MSGTYPE_REQUEST | PD_DATAROLE_UFP |
                    PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(1);
                request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(cfg->i)
                    | PD_RDO_FV_CURRENT_SET(cfg->i)
                    | PD_RDO_NO_USB_SUSPEND | PD_RDO_OBJPOS_SET(i + 1);
                return true;
            }
        }
    }
    /* Nothing matched, so get 5 V */
    request->hdr = PD_MSGTYPE_REQUEST | PD_DATAROLE_UFP |
        PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(1);
    request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(10)
        | PD_RDO_FV_CURRENT_SET(10)
        | PD_RDO_NO_USB_SUSPEND | PD_RDO_CAP_MISMATCH
        | PD_RDO_OBJPOS_SET(1);
    return false;
}

void pdb_dpm_get_sink_capability(union pd_msg *cap)
{
    /* Get the current configuration */
    struct pdb_config *cfg = pdb_config_flash_read();

    /* If we have no configuration, request 0.1 A at 5 V. */
    if (cfg == NULL) {
        /* Sink_Capabilities message */
        cap->hdr = PD_MSGTYPE_SINK_CAPABILITIES | PD_DATAROLE_UFP
            | PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(1);
        /* vSafe5V at the desired current. */
        cap->obj[0] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(100)
            | PD_PDO_SNK_FIXED_CURRENT_SET(10);
    /* If we want 5 V, we need to send only one PDO */
    } else if (cfg->v == 100) {
        /* Sink_Capabilities message */
        cap->hdr = PD_MSGTYPE_SINK_CAPABILITIES | PD_DATAROLE_UFP
            | PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(1);
        /* vSafe5V at the desired current. */
        cap->obj[0] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(cfg->v)
            | PD_PDO_SNK_FIXED_CURRENT_SET(cfg->i);
    /* Otherwise, send two PDOs, one for 5 V and one for the desired power. */
    } else {
        /* Sink_Capabilities message */
        cap->hdr = PD_MSGTYPE_SINK_CAPABILITIES | PD_DATAROLE_UFP
            | PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(2);
        /* First, vSafe5V.  100 mA, 5 V, and higher capability. */
        cap->obj[0] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(100)
            | PD_PDO_SNK_FIXED_CURRENT_SET(10);
        /* Next, desired_v and desired_i */
        cap->obj[1] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(cfg->v)
            | PD_PDO_SNK_FIXED_CURRENT_SET(cfg->i);
    }

    /* Set the unconstrained power flag. */
    if (dpm_unconstrained_power) {
        cap->obj[0] |= PD_PDO_SNK_FIXED_UNCONSTRAINED;
    }
}

void pdb_dpm_output_on(void)
{
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_MEDIUM_BLINK_OFF);
    palSetLine(LINE_OUT_CTRL);
}

void pdb_dpm_output_off(void)
{
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_ON);
    palClearLine(LINE_OUT_CTRL);
}
