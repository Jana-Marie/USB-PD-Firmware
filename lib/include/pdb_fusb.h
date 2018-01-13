/*
 * PD Buddy Firmware Library - USB Power Delivery for everyone
 * Copyright 2017-2018 Clayton G. Hobbs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PDB_FUSB_H
#define PDB_FUSB_H

#include <hal.h>

/* I2C addresses of the FUSB302B chips */
#define FUSB302B_ADDR 0x22
#define FUSB302B01_ADDR 0x23
#define FUSB302B10_ADDR 0x24
#define FUSB302B11_ADDR 0x25


/*
 * Configuration for the FUSB302B chip
 */
struct pdb_fusb_config {
    /* The I2C driver for the bus that the chip is connected to */
    I2CDriver *i2cp;
    /* The I2C address of the chip */
    i2caddr_t addr;
    /* The INT_N line */
    ioline_t int_n;
};

/*
 * FUSB Type-C Current level enum
 */
enum fusb_typec_current {
    fusb_tcc_none = 0,
    fusb_tcc_default = 1,
    fusb_tcc_1_5 = 2,
    fusb_sink_tx_ng = 2,
    fusb_tcc_3_0 = 3,
    fusb_sink_tx_ok = 3
};


#endif /* PDB_FUSB_H */
