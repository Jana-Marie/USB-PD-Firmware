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


/*
 * Configuration for the FUSB302B chip
 */
struct pdb_fusb_config {
    /* The I2C bus that the chip is connected to */
    I2CDriver *i2cp;
    /* The I2C address of the chip */
    i2caddr_t addr;
};


#endif /* PDB_FUSB_H */
