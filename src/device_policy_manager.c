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

#include "device_policy_manager.h"

#include <stdint.h>

#include <hal.h>

#include <pd.h>

#include "led.h"
#include "config.h"


/* The current draw when the output is disabled */
#define DPM_MIN_CURRENT PD_MA2PDI(30)


/*
 * Return the current specified by the given PDBS configuration object at the
 * given voltage (in millivolts), in centiamperes.
 */
static uint16_t dpm_get_current(struct pdbs_config *scfg, uint16_t mv)
{
    switch (scfg->flags & PDBS_CONFIG_FLAGS_CURRENT_DEFN) {
        case PDBS_CONFIG_FLAGS_CURRENT_DEFN_I:
            return scfg->i;
        case PDBS_CONFIG_FLAGS_CURRENT_DEFN_P:
            return (scfg->p * 1000 + mv - 1) / mv;
        case PDBS_CONFIG_FLAGS_CURRENT_DEFN_R:
            return (mv * 10 + scfg->r - 1) / scfg->r;
        default:
            /* In the case of an error, return a current that can't be
             * requested. */
            return PD_CA_MAX + 1;
    }
}


/*
 * Find the index of the first PDO from capabilities in the voltage range,
 * using the desired order.
 *
 * If there is no such PDO, returns -1 instead.
 */
static int8_t dpm_get_range_fixed_pdo_index(const union pd_msg *caps,
        struct pdbs_config *scfg)
{
    /* Get the number of PDOs */
    uint8_t numobj = PD_NUMOBJ_GET(caps);

    /* Get ready to iterate over the PDOs */
    int8_t i;
    int8_t step;
    if (scfg->flags & PDBS_CONFIG_FLAGS_HV_PREFERRED) {
        i = numobj - 1;
        step = -1;
    } else {
        i = 0;
        step = 1;
    }

    /* Look at the PDOs to see if one falls in our voltage range. */
    while (0 <= i && i < numobj) {
        /* If we have a fixed PDO, its V is within our range, and its I is at
         * least our desired I */
        uint16_t v = PD_PDO_SRC_FIXED_VOLTAGE_GET(caps->obj[i]);
        if ((caps->obj[i] & PD_PDO_TYPE) == PD_PDO_TYPE_FIXED
                && PD_PDO_SRC_FIXED_CURRENT_GET(caps->obj[i]) >= dpm_get_current(scfg, PD_PDV2MV(v))
                && v >= PD_MV2PDV(scfg->vmin)
                && v <= PD_MV2PDV(scfg->vmax)) {
            return i;
        }
        i += step;
    }
    return -1;
}

bool pdbs_dpm_evaluate_capability(struct pdb_config *cfg,
        const union pd_msg *caps, union pd_msg *request)
{
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    /* Update the stored Source_Capabilities */
    if (caps != NULL) {
        if (dpm_data->capabilities != NULL) {
            chPoolFree(&pdb_msg_pool, (union pd_msg *) dpm_data->capabilities);
        }
        dpm_data->capabilities = caps;
    } else {
        /* No new capabilities; use a shorter name for the stored ones. */
        caps = dpm_data->capabilities;
    }

    /* Get the current configuration */
    struct pdbs_config *scfg = pdbs_config_flash_read();
    /* Get the number of PDOs */
    uint8_t numobj = PD_NUMOBJ_GET(caps);

    /* Make the LED blink to indicate ongoing power negotiations */
    if (dpm_data->led_pd_status) {
        chEvtSignal(pdbs_led_thread, PDBS_EVT_LED_NEGOTIATING);
    }

    /* Get whether or not the power supply is constrained */
    dpm_data->_unconstrained_power = caps->obj[0] & PD_PDO_SRC_FIXED_UNCONSTRAINED;

    /* Get the current we want */
    uint16_t current = dpm_get_current(scfg, scfg->v);

    /* Make sure we have configuration */
    if (scfg != NULL && dpm_data->output_enabled) {
        /* Look at the PDOs to see if one matches our desires */
        for (uint8_t i = 0; i < numobj; i++) {
            /* If we have a fixed PDO, its V equals our desired V, and its I is
             * at least our desired I */
            if ((caps->obj[i] & PD_PDO_TYPE) == PD_PDO_TYPE_FIXED
                    && PD_PDO_SRC_FIXED_VOLTAGE_GET(caps->obj[i]) == PD_MV2PDV(scfg->v)
                    && PD_PDO_SRC_FIXED_CURRENT_GET(caps->obj[i]) >= current) {
                /* We got what we wanted, so build a request for that */
                request->hdr = cfg->pe.hdr_template | PD_MSGTYPE_REQUEST
                    | PD_NUMOBJ(1);
                if (scfg->flags & PDBS_CONFIG_FLAGS_GIVEBACK) {
                    /* GiveBack enabled */
                    request->obj[0] = PD_RDO_FV_MIN_CURRENT_SET(DPM_MIN_CURRENT)
                        | PD_RDO_FV_CURRENT_SET(current)
                        | PD_RDO_NO_USB_SUSPEND | PD_RDO_GIVEBACK
                        | PD_RDO_OBJPOS_SET(i + 1);
                } else {
                    /* GiveBack disabled */
                    request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(current)
                        | PD_RDO_FV_CURRENT_SET(current)
                        | PD_RDO_NO_USB_SUSPEND | PD_RDO_OBJPOS_SET(i + 1);
                }
                if (dpm_data->usb_comms) {
                    request->obj[0] |= PD_RDO_USB_COMMS;
                }

                /* Update requested voltage */
                dpm_data->_requested_voltage = PD_PDV2MV(PD_MV2PDV(scfg->v));

                dpm_data->_capability_match = true;
                return true;
            }
            /* If we have a PPS APDO, our desired V lies within its range, and
             * its I is at least our desired I */
            if ((caps->obj[i] & PD_PDO_TYPE) == PD_PDO_TYPE_AUGMENTED
                    && (caps->obj[i] & PD_APDO_TYPE) == PD_APDO_TYPE_PPS
                    && PD_APDO_PPS_MAX_VOLTAGE_GET(caps->obj[i]) >= PD_MV2PAV(scfg->v)
                    && PD_APDO_PPS_MIN_VOLTAGE_GET(caps->obj[i]) <= PD_MV2PAV(scfg->v)
                    && PD_APDO_PPS_CURRENT_GET(caps->obj[i]) >= PD_CA2PAI(current)) {
                /* We got what we wanted, so build a request for that */
                request->hdr = cfg->pe.hdr_template | PD_MSGTYPE_REQUEST
                    | PD_NUMOBJ(1);

                /* Build a request */
                request->obj[0] = PD_RDO_PROG_CURRENT_SET(PD_CA2PAI(current))
                    | PD_RDO_PROG_VOLTAGE_SET(PD_MV2PRV(scfg->v))
                    | PD_RDO_NO_USB_SUSPEND | PD_RDO_OBJPOS_SET(i + 1);
                if (dpm_data->usb_comms) {
                    request->obj[0] |= PD_RDO_USB_COMMS;
                }

                /* Update requested voltage */
                dpm_data->_requested_voltage = PD_PRV2MV(PD_MV2PRV(scfg->v));

                dpm_data->_capability_match = true;
                return true;
            }
        }
        /* If there's a PDO in the voltage range, use it */
        int8_t i = dpm_get_range_fixed_pdo_index(caps, scfg);
        if (i >= 0) {
            /* We got what we wanted, so build a request for that */
            request->hdr = cfg->pe.hdr_template | PD_MSGTYPE_REQUEST
                | PD_NUMOBJ(1);
            /* Get the current we need at this voltage */
            current = dpm_get_current(scfg, PD_PDV2MV(PD_PDO_SRC_FIXED_VOLTAGE_GET(caps->obj[i])));
            if (scfg->flags & PDBS_CONFIG_FLAGS_GIVEBACK) {
                /* GiveBack enabled */
                request->obj[0] = PD_RDO_FV_MIN_CURRENT_SET(DPM_MIN_CURRENT)
                    | PD_RDO_FV_CURRENT_SET(current)
                    | PD_RDO_NO_USB_SUSPEND | PD_RDO_GIVEBACK
                    | PD_RDO_OBJPOS_SET(i + 1);
            } else {
                /* GiveBack disabled */
                request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(current)
                    | PD_RDO_FV_CURRENT_SET(current)
                    | PD_RDO_NO_USB_SUSPEND | PD_RDO_OBJPOS_SET(i + 1);
            }
            if (dpm_data->usb_comms) {
                request->obj[0] |= PD_RDO_USB_COMMS;
            }

            /* Update requested voltage */
            dpm_data->_requested_voltage = PD_PDV2MV(PD_PDO_SRC_FIXED_VOLTAGE_GET(caps->obj[i]));

            dpm_data->_capability_match = true;
            return true;
        }
    }
    /* Nothing matched (or no configuration), so get 5 V at low current */
    request->hdr = cfg->pe.hdr_template | PD_MSGTYPE_REQUEST | PD_NUMOBJ(1);
    request->obj[0] = PD_RDO_FV_MAX_CURRENT_SET(DPM_MIN_CURRENT)
        | PD_RDO_FV_CURRENT_SET(DPM_MIN_CURRENT)
        | PD_RDO_NO_USB_SUSPEND
        | PD_RDO_OBJPOS_SET(1);
    /* If the output is enabled and we got here, it must be a capability
     * mismatch. */
    if (dpm_data->output_enabled) {
        request->obj[0] |= PD_RDO_CAP_MISMATCH;
    }
    /* If we can do USB communications, tell the power supply */
    if (dpm_data->usb_comms) {
        request->obj[0] |= PD_RDO_USB_COMMS;
    }

    /* Update requested voltage */
    dpm_data->_requested_voltage = 5000;

    /* At this point, we have a capability match iff the output is disabled */
    dpm_data->_capability_match = !dpm_data->output_enabled;
    return !dpm_data->output_enabled;
}

void pdbs_dpm_get_sink_capability(struct pdb_config *cfg, union pd_msg *cap)
{
    /* Keep track of how many PDOs we've added */
    int numobj = 0;
    /* Get the current configuration */
    struct pdbs_config *scfg = pdbs_config_flash_read();
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    /* If we have no configuration or want something other than 5 V, add a PDO
     * for vSafe5V */
    if (scfg == NULL || PD_MV2PDV(scfg->v) != PD_MV2PDV(5000)) {
        /* Minimum current, 5 V, and higher capability. */
        cap->obj[numobj++] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(PD_MV2PDV(5000))
            | PD_PDO_SNK_FIXED_CURRENT_SET(DPM_MIN_CURRENT);
    }

    if (scfg != NULL) {
        /* Get the current we want */
        uint16_t current = dpm_get_current(scfg, scfg->v);
        /* Add a PDO for the desired power. */
        cap->obj[numobj++] = PD_PDO_TYPE_FIXED
            | PD_PDO_SNK_FIXED_VOLTAGE_SET(PD_MV2PDV(scfg->v))
            | PD_PDO_SNK_FIXED_CURRENT_SET(current);

        /* Get the PDO from the voltage range */
        int8_t i = dpm_get_range_fixed_pdo_index(dpm_data->capabilities, scfg);

        /* If it's vSafe5V, set our vSafe5V's current to what we want */
        if (i == 0) {
            cap->obj[0] &= ~PD_PDO_SNK_FIXED_CURRENT;
            cap->obj[0] |= PD_PDO_SNK_FIXED_CURRENT_SET(current);
        } else {
            /* If we want more than 5 V, set the Higher Capability flag */
            if (PD_MV2PDV(scfg->v) != PD_MV2PDV(5000)) {
                cap->obj[0] |= PD_PDO_SNK_FIXED_HIGHER_CAP;
            }

            /* If the range PDO is a different voltage than the preferred
             * voltage, add it to the array. */
            if (i > 0 && PD_PDO_SRC_FIXED_VOLTAGE_GET(dpm_data->capabilities->obj[i]) != PD_MV2PDV(scfg->v)) {
                cap->obj[numobj++] = PD_PDO_TYPE_FIXED
                    | PD_PDO_SNK_FIXED_VOLTAGE_SET(PD_PDO_SRC_FIXED_VOLTAGE_GET(dpm_data->capabilities->obj[i]))
                    | PD_PDO_SNK_FIXED_CURRENT_SET(PD_PDO_SRC_FIXED_CURRENT_GET(dpm_data->capabilities->obj[i]));
            }

            /* If we have three PDOs at this point, make sure the last two are
             * sorted by voltage. */
            if (numobj == 3
                    && (cap->obj[1] & PD_PDO_SNK_FIXED_VOLTAGE)
                    > (cap->obj[2] & PD_PDO_SNK_FIXED_VOLTAGE)) {
                cap->obj[1] ^= cap->obj[2];
                cap->obj[2] ^= cap->obj[1];
                cap->obj[1] ^= cap->obj[2];
            }
        }

        /* If we're using PD 3.0, add a PPS APDO for our desired voltage */
        if ((cfg->pe._message->hdr & PD_HDR_SPECREV) >= PD_SPECREV_3_0) {
            cap->obj[numobj++] = PD_PDO_TYPE_AUGMENTED | PD_APDO_TYPE_PPS
                | PD_APDO_PPS_MAX_VOLTAGE_SET(PD_MV2PAV(scfg->v))
                | PD_APDO_PPS_MIN_VOLTAGE_SET(PD_MV2PAV(scfg->v))
                | PD_APDO_PPS_CURRENT_SET(PD_CA2PAI(current));
        }
    }

    /* Set the unconstrained power flag. */
    if (dpm_data->_unconstrained_power) {
        cap->obj[0] |= PD_PDO_SNK_FIXED_UNCONSTRAINED;
    }

    /* Set the USB communications capable flag. */
    if (dpm_data->usb_comms) {
        cap->obj[0] |= PD_PDO_SNK_FIXED_USB_COMMS;
    }

    /* Set the Sink_Capabilities message header */
    cap->hdr = cfg->pe.hdr_template | PD_MSGTYPE_SINK_CAPABILITIES
        | PD_NUMOBJ(numobj);
}

bool pdbs_dpm_giveback_enabled(struct pdb_config *cfg)
{
    (void) cfg;
    struct pdbs_config *scfg = pdbs_config_flash_read();

    return scfg->flags & PDBS_CONFIG_FLAGS_GIVEBACK;
}

bool pdbs_dpm_evaluate_typec_current(struct pdb_config *cfg,
        enum fusb_typec_current tcc)
{
    struct pdbs_config *scfg = pdbs_config_flash_read();
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    /* We don't control the voltage anymore; it will always be 5 V. */
    dpm_data->_requested_voltage = 5000;

    /* Make the present Type-C Current advertisement available to the rest of
     * the DPM */
    dpm_data->typec_current = tcc;

    /* If we have no configuration or don't want 5 V, Type-C Current can't
     * possibly satisfy our needs */
    if (scfg == NULL || (scfg->v != 5000 && (scfg->vmin > 5000
            || scfg->vmax < 5000))) {
        dpm_data->_capability_match = false;
        return false;
    }

    /* Get the current we want */
    uint16_t current = dpm_get_current(scfg, 5000);

    /* If 1.5 A is available and we want no more than that, great. */
    if (tcc == fusb_tcc_1_5 && current <= 150) {
        dpm_data->_capability_match = true;
        return true;
    }
    /* If 3 A is available and we want no more than that, that's great too. */
    if (tcc == fusb_tcc_3_0 && current <= 300) {
        dpm_data->_capability_match = true;
        return true;
    }
    /* We're overly cautious if USB default current is available, since that
     * could mean different things depending on the port we're connected to,
     * and since we're really supposed to enumerate in order to request more
     * than 100 mA.  This could be changed in the future. */

    dpm_data->_capability_match = false;
    return false;
}

void pdbs_dpm_pd_start(struct pdb_config *cfg)
{
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    if (dpm_data->led_pd_status) {
        chEvtSignal(pdbs_led_thread, PDBS_EVT_LED_NEGOTIATING);
    }
}

/*
 * Set the output state, with LED indication.
 */
static void dpm_output_set(struct pdbs_dpm_data *dpm_data, bool state)
{
    /* Update the present voltage */
    dpm_data->_present_voltage = dpm_data->_requested_voltage;

    /* Set the power output */
    if (state && dpm_data->output_enabled) {
        /* Turn the output on */
        if (dpm_data->led_pd_status) {
            chEvtSignal(pdbs_led_thread, PDBS_EVT_LED_OUTPUT_ON);
        }
        palSetLine(LINE_OUT_CTRL);
    } else {
        /* Turn the output off */
        if (dpm_data->led_pd_status) {
            chEvtSignal(pdbs_led_thread, PDBS_EVT_LED_OUTPUT_OFF);
        }
        palClearLine(LINE_OUT_CTRL);
    }
}

void pdbs_dpm_transition_default(struct pdb_config *cfg)
{
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    /* Pretend we requested 5 V */
    dpm_data->_requested_voltage = 5000;
    /* Turn the output off */
    dpm_output_set(cfg->dpm_data, false);
}

void pdbs_dpm_transition_min(struct pdb_config *cfg)
{
    dpm_output_set(cfg->dpm_data, false);
}

void pdbs_dpm_transition_standby(struct pdb_config *cfg)
{
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    /* If the voltage is changing, enter Sink Standby */
    if (dpm_data->_requested_voltage != dpm_data->_present_voltage) {
        /* For the PD Buddy Sink, entering Sink Standby is equivalent to
         * turning the output off.  However, we don't want to change the LED
         * state for standby mode. */
        palClearLine(LINE_OUT_CTRL);
    }
}

void pdbs_dpm_transition_requested(struct pdb_config *cfg)
{
    /* Cast the dpm_data to the right type */
    struct pdbs_dpm_data *dpm_data = cfg->dpm_data;

    dpm_output_set(cfg->dpm_data, dpm_data->_capability_match);
}
