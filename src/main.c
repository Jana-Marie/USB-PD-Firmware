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

#include "usbcfg.h"

#include <pdb.h>
#include <pd.h>
#include "device_policy_manager.h"
#include "stm32f072_bootloader.h"
#include "config.h"
#include "chprintf.h"

void (*func)(BaseSequentialStream *chp, int argc, char *argv[]);


#define ADC_GRP1_NUM_CHANNELS   1
#define ADC_GRP1_BUF_DEPTH      1
static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN10.
 */
static const ADCConversionGroup adcgrpcfg1 = {
    FALSE,
    ADC_GRP1_NUM_CHANNELS,
    NULL,
    NULL,
    ADC_CFGR1_CONT | ADC_CFGR1_RES_12BIT,             /* CFGR1 */
    ADC_TR(0, 0),                                     /* TR */
    ADC_SMPR_SMP_28P5,                                 /* SMPR */
    ADC_CHSELR_CHSEL5           /* CHSELR */
};



static PWMConfig pwmcfg = {
    1000,                                    /* 10kHz PWM clock frequency.     */
    200,                                    /* Initial PWM period 1S.         */
    NULL,                                     /* Period callback.               */
    {
        {PWM_OUTPUT_ACTIVE_HIGH, NULL},          /* CH1 mode and callback.         */
        {PWM_OUTPUT_ACTIVE_HIGH, NULL},             /* CH2 mode and callback.         */
        {PWM_OUTPUT_ACTIVE_HIGH, NULL},             /* CH3 mode and callback.         */
        {PWM_OUTPUT_ACTIVE_HIGH, NULL}              /* CH4 mode and callback.         */
    },
    0,                                        /* Control Register 2.            */
};

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

const struct pdbs_shell_cfg shell_cfg = {
    (BaseSequentialStream *)&SDU1
};


/*
 * PD Buddy Sink DPM data
 */
static struct pdbs_dpm_data dpm_data = {
    NULL,
    fusb_tcc_none,
    true,
    true,
    false,
    ._present_voltage = 5000
};

/*
 * PD Buddy firmware library configuration object
 */
static struct pdb_config pdb_config = {
    .fusb = {
        &I2CD2,
        FUSB302B_ADDR,
        LINE_INT_N
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
        pdbs_dpm_transition_typec,
        NULL /* not_supported_received */
    },
    .dpm_data = &dpm_data,
    .state = 4
};

/*
 * Negotiate with the power supply for the configured power
 */
static void sink(void)
{


    dpm_data.output_enabled = true;
    //dpm_data.usb_comms = true;

    /* Start the USB Power Delivery threads */
    pdb_init(&pdb_config);

    //palSetLine(LINE_PWM);

    /* Disconnect from USB */
    usbDisconnectBus(serusbcfg.usbp);

    /* Start the USB serial interface */
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    /* Start USB */
    chThdSleepMilliseconds(100);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    BaseSequentialStream *chp = shell_cfg.io;

    palSetGroupMode(GPIOA, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2)| PAL_PORT_BIT(3) | PAL_PORT_BIT(4) | PAL_PORT_BIT(5)| PAL_PORT_BIT(6) | PAL_PORT_BIT(7) | PAL_PORT_BIT(8),0, PAL_MODE_INPUT_ANALOG); // this is 15th channel

    adcStart(&ADCD1, NULL);
    //adcSTM32SetCCR(ADC_CCR_VBATEN | ADC_CCR_VREFEN);

    /*
     * Linear conversion.
     */
    adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
    chThdSleepMilliseconds(1000);

    //palSetPadMode(GPIOB, 3, PAL_MODE_ALTERNATE(2));
    //pwmStart(&PWMD2, &pwmcfg);
    //pwmEnableChannel(&PWMD2, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD2, 8000));

    /* Wait, letting all the other threads do their work. */
    //HAL_ADC_Start(&hadc1);

     adcAcquireBus  (&ADCD1);

    while (true) {
        chThdSleepMilliseconds(100);

        //pwmChangePeriodI(&PWMD2, PWM_PERCENTAGE_TO_WIDTH(&PWMD2, 2000));
        //pwmEnableChannel(&PWMD2, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD2, cnt));

        //cnt += 100;
        //if (cnt >= 10000) cnt = 0;

        chprintf(chp, " %d \n\r",samples1[0]); 
        //adcStartConversion(&ADCD1, &adccg, samples_buf, ADC_BUF_DEPTH);
        adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
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

    /* Start I2C2 to make communication with the PHY possible */
    i2cStart(pdb_config.fusb.i2cp, &i2c2config);


    /* *chirp* */
    if (palReadLine(LINE_BUTTON1) == PAL_HIGH) {
        dfu_run_bootloader();
    }

    /* Button unpressed -> deliver power, buddy! */
    sink();
}
