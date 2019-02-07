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

#ifndef PDBS_CONFIG_H
#define PDBS_CONFIG_H

#include <stdint.h>

#include <ch.h>
#include <hal.h>


/*
 * PD Buddy Sink configuration structure
 */
struct pdbs_config {
    /* Status halfword, used to indicate which config objects can be written to
     * and which one is valid. */
    uint16_t status;
    /* Flags halfword for miscellaneous small fields. */
    uint16_t flags;
    /* Preferred voltage, in millivolts. */
    uint16_t v;
    /* Union for specifying how much current to request. */
    union {
        /* Required current, in centiamperes. */
        uint16_t i;
        /* Required power, in centiwatts. */
        uint16_t p;
        /* Value of resistive load, in centiohms. */
        uint16_t r;
    };
    /* Lower end of voltage range, in millivolts. */
    uint16_t vmin;
    /* Upper end of voltage range, in millivolts. */
    uint16_t vmax;
    /* Extra bytes reserved for future use. */
    uint16_t _reserved[2];
} __attribute__((packed));


struct pdbs_shell_cfg {
    BaseSequentialStream *io;
};


/* Status for configuration structures.  EMPTY indicates that the struct is
 * ready to be written, including a status update to VALID.  Once the struct is
 * no longer needed, the status is updated to INVALID.  Erasing the flash page
 * resets all structures to EMPTY. */
#define PDBS_CONFIG_STATUS_INVALID 0x0000
#define PDBS_CONFIG_STATUS_VALID 0xBEEF
#define PDBS_CONFIG_STATUS_EMPTY 0xFFFF

/* Flags for configuration structures. */
/* GiveBack supported */
#define PDBS_CONFIG_FLAGS_GIVEBACK (1 << 0)
/* Variable and battery PDOs preferred (FIXME: not implemented) */
#define PDBS_CONFIG_FLAGS_VAR_BAT (1 << 1)
/* High voltages preferred */
#define PDBS_CONFIG_FLAGS_HV_PREFERRED (1 << 2)
/* Current definition type */
#define PDBS_CONFIG_FLAGS_CURRENT_DEFN_SHIFT 3
#define PDBS_CONFIG_FLAGS_CURRENT_DEFN (0x3 << PDBS_CONFIG_FLAGS_CURRENT_DEFN_SHIFT)
#define PDBS_CONFIG_FLAGS_CURRENT_DEFN_I (0 << PDBS_CONFIG_FLAGS_CURRENT_DEFN_SHIFT)
#define PDBS_CONFIG_FLAGS_CURRENT_DEFN_P (1 << PDBS_CONFIG_FLAGS_CURRENT_DEFN_SHIFT)
#define PDBS_CONFIG_FLAGS_CURRENT_DEFN_R (2 << PDBS_CONFIG_FLAGS_CURRENT_DEFN_SHIFT)


/* Flash configuration array */
extern struct pdbs_config *pdbs_config_array;

/* The number of elements in the pdbs_config_array */
#define PDBS_CONFIG_ARRAY_LEN 128


/*
 * Print a struct pdbs_config to the given BaseSequentialStream
 */
void pdbs_config_print(BaseSequentialStream *chp, const struct pdbs_config *cfg);

/*
 * Erase the flash page used for configuration
 */
void pdbs_config_flash_erase(void);

/*
 * Write a configuration structure to flash, invalidating the previous
 * configuration.  If necessary, the flash page is erased before writing the
 * new structure.
 */
void pdbs_config_flash_update(const struct pdbs_config *cfg);

/*
 * Get the first valid configuration strucure.  If the flash page is empty,
 * return NULL instead.
 *
 * The location of the configuration is cached, and the cache is updated when
 * pdbs_config_flash_erase and pdbs_config_flash_update are called.  The full
 * lookup is only performed the first time this function is called, so there's
 * very little penalty to calling it repeatedly.
 */
struct pdbs_config *pdbs_config_flash_read(void);


#endif /* PDBS_CONFIG_H */
