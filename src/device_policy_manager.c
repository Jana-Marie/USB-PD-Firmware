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
#include "fusb302b.h"


/* The current draw when the output is disabled */
#define DPM_MIN_CURRENT PD_MA2PDI(100)

/* Whether or not the power supply is unconstrained */
static bool dpm_unconstrained_power;

/* The last explicitly or implicitly negotiated voltage in PDV */
static int dpm_present_voltage = PD_MV2PDV(5000);

/* The requested voltage */
static int dpm_requested_voltage;

bool pdb_dpm_evaluate_capability(const union pd_msg *capabilities, union pd_msg *request)
{
    /* Get the current configuration */
    struct pdb_config *cfg = pdb_config_flash_read();
    /* Get the number of PDOs */
    uint8_t numobj = PD_NUMOBJ_GET(capabilities);

    /* Make the LED blink to indicate ongoing power negotiations */
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_FAST_BLINK);

    /* Get whether or not the power supply is constrained */
    dpm_unconstrained_power = capabilities->obj[0] & PD_PDO_SRC_FIXED_UNCONSTRAINED;

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
                if (cfg->flags & PDB_CONFIG_FLAGS_GIVEBACK) {
                    /* GiveBack enabled */
                    request->obj[0] = PD_RDO_FV_MIN_CURRENT_SET(DPM_MIN_CURRENT)
                        | PD_RDO_FV_CURRENT_SET(cfg->i)
                        | PD_RDO_NO_USB_SUSPEND | PD_RDO_GIVEBACK
                        | PD_RDO_OBJPOS_SET(i + 1);
                } else {
                    /* GiveBack disabled */
                    request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(cfg->i)
                        | PD_RDO_FV_CURRENT_SET(cfg->i)
                        | PD_RDO_NO_USB_SUSPEND | PD_RDO_OBJPOS_SET(i + 1);
                }

                /* Update requested voltage */
                dpm_requested_voltage = cfg->v;

                return true;
            }
        }
    }
    /* Nothing matched (or no configuration), so get 5 V at low current */
    request->hdr = PD_MSGTYPE_REQUEST | PD_DATAROLE_UFP |
        PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(1);
    request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(DPM_MIN_CURRENT)
        | PD_RDO_FV_CURRENT_SET(DPM_MIN_CURRENT)
        | PD_RDO_NO_USB_SUSPEND | PD_RDO_CAP_MISMATCH
        | PD_RDO_OBJPOS_SET(1);

    /* Update requested voltage */
    dpm_requested_voltage = PD_MV2PDV(5000);

    return false;
}

void pdb_dpm_get_sink_capability(union pd_msg *cap)
{
    /* Keep track of how many PDOs we've added */
    int numobj = 0;
    /* Get the current configuration */
    struct pdb_config *cfg = pdb_config_flash_read();

    /* If we have no configuration or want something other than 5 V, add a PDO
     * for vSafe5V */
    if (cfg == NULL || cfg->v != 100) {
        /* Minimum current, 5 V, and higher capability. */
        cap->obj[numobj++] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(100)
            | PD_PDO_SNK_FIXED_CURRENT_SET(DPM_MIN_CURRENT);
    }

    /* Add a PDO for the desired power. */
    if (cfg != NULL) {
        cap->obj[numobj++] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(cfg->v)
            | PD_PDO_SNK_FIXED_CURRENT_SET(cfg->i);
        /* If we want more than 5 V, set the Higher Capability flag */
        if (cfg->v != 100) {
            cap->obj[0] |= PD_PDO_SNK_FIXED_HIGHER_CAP;
        }
    }

    /* Set the unconstrained power flag. */
    if (dpm_unconstrained_power) {
        cap->obj[0] |= PD_PDO_SNK_FIXED_UNCONSTRAINED;
    }

    /* Set the Sink_Capabilities message header */
    cap->hdr = PD_MSGTYPE_SINK_CAPABILITIES | PD_DATAROLE_UFP | PD_SPECREV_2_0
        | PD_POWERROLE_SINK | PD_NUMOBJ(numobj);
}

bool pdb_dpm_giveback_enabled(void)
{
    struct pdb_config *cfg = pdb_config_flash_read();

    return cfg->flags & PDB_CONFIG_FLAGS_GIVEBACK;
}

bool pdb_dpm_evaluate_typec_current(void)
{
    static bool cfg_set = false;
    static struct pdb_config *cfg = NULL;

    /* Only get the configuration the first time this function runs, since its
     * location will never change without rebooting into setup mode. */
    if (!cfg_set) {
        cfg = pdb_config_flash_read();
        cfg_set = true;
    }

    /* We don't control the voltage anymore; it will always be 5 V. */
    dpm_requested_voltage = PD_MV2PDV(5000);

    /* If we have no configuration or don't want 5 V, Type-C Current can't
     * possibly satisfy our needs */
    if (cfg == NULL || cfg->v != 100) {
        return false;
    }

    /* Get the present Type-C Current advertisement */
    enum fusb_typec_current tcc = fusb_get_typec_current();

    /* If 1.5 A is available and we want no more than that, great. */
    if (tcc == OnePointFiveAmps && cfg->i <= 150) {
        return true;
    }
    /* If 3 A is available and we want no more than that, that's great too. */
    if (tcc == ThreePointZeroAmps && cfg->i <= 300) {
        return true;
    }
    /* We're overly cautious if USB default current is available, since that
     * could mean different things depending on the port we're connected to,
     * and since we're really supposed to enumerate in order to request more
     * than 100 mA.  This could be changed in the future. */

    return false;
}

void pdb_dpm_pd_start(void)
{
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_FAST_BLINK);
}

void pdb_dpm_sink_standby(void)
{
    /* If the voltage is changing, enter Sink Standby */
    if (dpm_requested_voltage != dpm_present_voltage) {
        /* For the PD Buddy Sink, entering Sink Standby is equivalent to
         * turning the output off.  However, we don't want to change the LED
         * state for standby mode. */
        palClearLine(LINE_OUT_CTRL);
    }
}

void pdb_dpm_output_set(bool state)
{
    /* Update the present voltage */
    dpm_present_voltage = dpm_requested_voltage;

    /* Set the power output */
    if (state) {
        /* Turn the output on */
        chEvtSignal(pdb_led_thread, PDB_EVT_LED_MEDIUM_BLINK_OFF);
        palSetLine(LINE_OUT_CTRL);
    } else {
        /* Turn the output off */
        chEvtSignal(pdb_led_thread, PDB_EVT_LED_ON);
        palClearLine(LINE_OUT_CTRL);
    }
}

void pdb_dpm_output_default(void)
{
    /* Pretend we requested 5 V */
    dpm_requested_voltage = PD_MV2PDV(5000);
    /* Turn the output off */
    pdb_dpm_output_set(false);
}
