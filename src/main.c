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

/*
   ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <ch.h>
#include <hal.h>

#include "shell.h"
#include "usbcfg.h"

#include <pdb.h>
#include "led.h"
#include "device_policy_manager.h"

/*
 * I2C configuration object.
 * I2C2_TIMINGR: 1000 kHz with I2CCLK = 48 MHz, rise time = 100 ns,
 *               fall time = 10 ns (0x00700818)
 */
static const I2CConfig i2c2config = {
    0x00700818,
    0,
    0
};

/*
 * PD Buddy firmware library configuration object
 */
static struct pdb_config pdb_config = {
    .fusb = {
        &I2CD2,
        FUSB302B_ADDR
    },
    .dpm = {
        pdbs_dpm_evaluate_capability,
        pdbs_dpm_get_sink_capability,
        pdbs_dpm_giveback_enabled,
        pdbs_dpm_evaluate_typec_current,
        pdbs_dpm_pd_start,
        pdbs_dpm_transition_default,
        pdbs_dpm_transition_min,
        pdbs_dpm_transition_standby,
        pdbs_dpm_transition_requested,
        pdbs_dpm_transition_requested /* XXX type-c current */
    }
};

/*
 * Enter setup mode
 */
static void setup(void)
{
    /* Configure the DPM to play nice with the shell */
    pdb_dpm_output_enabled = false;
    pdb_dpm_led_pd_status = false;
    pdb_dpm_usb_comms = true;

    /* Start the USB Power Delivery threads */
    pdb_init(&pdb_config);

    /* Indicate that we're in setup mode */
    chEvtSignal(pdbs_led_thread, PDBS_EVT_LED_CONFIG);

    /* Disconnect from USB */
    usbDisconnectBus(serusbcfg.usbp);

    /* Start the USB serial interface */
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    /* Start USB */
    chThdSleepMilliseconds(100);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    /* Start the shell */
    pdbs_shell();
}

/*
 * Negotiate with the power supply for the configured power
 */
static void sink(void)
{
    /* Start the USB Power Delivery threads */
    pdb_init(&pdb_config);

    /* Wait, letting all the other threads do their work. */
    while (true) {
        chThdSleepMilliseconds(1000);
    }
}

/*
 * Application entry point.
 */
int main(void) {

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();

    /* Create the LED thread. */
    pdbs_led_run();

    /* Start I2C2 to make communication with the PHY possible */
    i2cStart(&I2CD2, &i2c2config);

    /* Decide what mode to enter by the state of the button */
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
        /* Button pressed -> setup mode */
        setup();
    } else {
        /* Button unpressed -> deliver power, buddy! */
        sink();
    }
}
