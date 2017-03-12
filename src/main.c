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

#include "priorities.h"
#include "led.h"
#include "policy_engine.h"
#include "protocol_tx.h"
#include "protocol_rx.h"
#include "hard_reset.h"
#include "int_n.h"
#include "fusb302b.h"
#include "messages.h"

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

static void setup(void)
{
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_SLOW_BLINK);

    /* TODO: implement the configuration mode */

    while (true) {
        chThdSleepMilliseconds(1000);
    }
}

static void pd_buddy(void)
{
    chEvtSignal(pdb_led_thread, PDB_EVT_LED_FAST_BLINK);

    /* Start I2C2 to make communication with the PHY possible */
    i2cStart(&I2CD2, &i2c2config);

    /* Initialize the FUSB302B */
    fusb_setup();

    /* Create the policy engine thread. */
    pdb_pe_run();

    /* Create the protocol layer threads. */
    pdb_prlrx_run();
    pdb_prltx_run();
    pdb_hardrst_run();

    /* Create the INT_N thread. */
    pdb_int_n_run();

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

    /* Set up the free messages mailbox */
    pdb_msg_pool_init();

    /* Create the LED thread. */
    pdb_led_run();

    /* Decide what mode to enter by the state of the button */
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
        /* Button pressed -> setup mode */
        setup();
    } else {
        /* Button unpressed -> deliver power, buddy! */
        pd_buddy();
    }
}
