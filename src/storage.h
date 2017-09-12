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

#ifndef PDB_STORAGE_H
#define PDB_STORAGE_H

#include <stdint.h>

#include <ch.h>


/*
 * PD Buddy Sink configuration structure
 */
struct pdb_config {
    uint16_t status;
    uint16_t flags;
    uint16_t v;
    uint16_t i;
    uint16_t v_min;
    uint16_t v_max;
    uint16_t _reserved[2];
} __attribute__((packed));

/* Status for configuration structures.  EMPTY indicates that the struct is
 * ready to be written, including a status update to VALID.  Once the struct is
 * no longer needed, the status is updated to INVALID.  Erasing the flash page
 * resets all structures to EMPTY. */
#define PDB_CONFIG_STATUS_INVALID 0x0000
#define PDB_CONFIG_STATUS_VALID 0xBEEF
#define PDB_CONFIG_STATUS_EMPTY 0xFFFF

/* Flags for configuration structures. */
/* GiveBack supported */
#define PDB_CONFIG_FLAGS_GIVEBACK 0x0001
/* Variable and battery PDOs supported (v_min and v_max valid) */
#define PDB_CONFIG_FLAGS_VAR_BAT 0x0002


/* Flash configuration array */
extern struct pdb_config *pdb_config_array;

/* The number of elements in the pdb_config_array */
#define PDB_CONFIG_ARRAY_LEN 128


/*
 * Print a struct pdb_config to the given BaseSequentialStream
 */
void pdb_config_print(BaseSequentialStream *chp, const struct pdb_config *cfg);

/*
 * Erase the flash page used for configuration
 */
void pdb_config_flash_erase(void);

/*
 * Write a configuration structure to flash, invalidating the previous
 * configuration.  If necessary, the flash page is erased before writing the
 * new structure.
 */
void pdb_config_flash_update(const struct pdb_config *cfg);

/*
 * Get the first valid configuration strucure.  If the flash page is empty,
 * return NULL instead.
 *
 * The location of the configuration is cached, and the cache is updated when
 * pdb_config_flash_erase and pdb_config_flash_update are called.  The full
 * lookup is only performed the first time this function is called, so there's
 * very little penalty to calling it repeatedly.
 */
struct pdb_config *pdb_config_flash_read(void);


#endif /* PDB_STORAGE_H */
