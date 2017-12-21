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
    /* The I2C bus that the chip is connected to */
    I2CDriver *i2cp;
    /* The I2C address of the chip */
    i2caddr_t addr;
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
