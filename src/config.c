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

#include "config.h"

#include "chprintf.h"

#include <pd.h>


/* Initialize the location of the configuration array.  PDBS_CONFIG_BASE is set
 * in the Makefile. */
struct pdbs_config *pdbs_config_array = (struct pdbs_config *) PDBS_CONFIG_BASE;

/* The location of the current configuration object.  NULL if not known or
 * there is no current configuration. */
struct pdbs_config *config_cur = NULL;


void pdbs_config_print(BaseSequentialStream *chp, const struct pdbs_config *cfg)
{
    /* Print the status */
    chprintf(chp, "status: ");
    switch (cfg->status) {
        case PDBS_CONFIG_STATUS_INVALID:
            chprintf(chp, "in");
            /* fall-through */
        case PDBS_CONFIG_STATUS_VALID:
            chprintf(chp, "valid\r\n");
            break;
        case PDBS_CONFIG_STATUS_EMPTY:
            chprintf(chp, "empty\r\n");
            /* Stop early because the rest of the information is meaningless in
             * this case. */
            return;
        default:
            chprintf(chp, "%04X\r\n", cfg->status);
            break;
    }

    /* Print the flags */
    chprintf(chp, "flags: ");
    if (cfg->flags == 0) {
        chprintf(chp, "(none)");
    }
    if (cfg->flags & PDBS_CONFIG_FLAGS_GIVEBACK) {
        chprintf(chp, "GiveBack ");
    }
    if (cfg->flags & PDBS_CONFIG_FLAGS_VAR_BAT) {
        chprintf(chp, "Var/Bat ");
    }
    chprintf(chp, "\r\n");

    /* Print voltages and current */
    chprintf(chp, "v: %d.%02d V\r\n", PD_PDV_V(cfg->v), PD_PDV_CV(cfg->v));
    chprintf(chp, "i: %d.%02d A\r\n", PD_PDI_A(cfg->i), PD_PDI_CA(cfg->i));
    if (cfg->flags & PDBS_CONFIG_FLAGS_VAR_BAT) {
        chprintf(chp, "vmin: %d.%02d V\r\n", PD_PDV_V(cfg->vmin),
                 PD_PDV_CV(cfg->vmin));
        chprintf(chp, "vmax: %d.%02d V\r\n", PD_PDV_V(cfg->vmax),
                 PD_PDV_CV(cfg->vmax));
    }
}

/*
 * Unlock the flash interface
 */
static void flash_unlock(void)
{
    /* Wait till no operation is on going */
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {
        /* Note: we might want a timeout here */
    }

    /* Check that the Flash is locked */
    if ((FLASH->CR & FLASH_CR_LOCK) != 0) {
        /* Perform unlock sequence */
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

/*
 * Lock the flash interface
 */
static void flash_lock(void)
{
    /* Wait till no operation is on going */
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {
        /* Note: we might want a timeout here */
    }

    /* Check that the Flash is unlocked */
    if ((FLASH->CR & FLASH_CR_LOCK) == 0) {
        /* Lock the flash */
        FLASH->CR |= FLASH_CR_LOCK;
    }
}

/*
 * Write one halfword to flash
 */
static void flash_write_halfword(uint16_t *addr, uint16_t data)
{
    /* Set the PG bit in the FLASH_CR register to enable programming */
    FLASH->CR |= FLASH_CR_PG;
    /* Perform the data write (half-word) at the desired address */
    *(__IO uint16_t*)(addr) = data;
    /* Wait until the BSY bit is reset in the FLASH_SR register */
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {
        /* For robust implementation, add here time-out management */
    }
    /* Check the EOP flag in the FLASH_SR register */
    if ((FLASH->SR & FLASH_SR_EOP) != 0) {
        /* clear it by software by writing it at 1 */
        FLASH->SR = FLASH_SR_EOP;
    } else {
        /* Manage the error cases */
    }
    /* Reset the PG Bit to disable programming */
    FLASH->CR &= ~FLASH_CR_PG;
}

/*
 * Erase the configuration page, without any locking
 */
static void flash_erase(void)
{
    /* Set the PER bit in the FLASH_CR register to enable page erasing */
    FLASH->CR |= FLASH_CR_PER;
    /* Program the FLASH_AR register to select a page to erase */
    FLASH->AR = (int) pdbs_config_array;
    /* Set the STRT bit in the FLASH_CR register to start the erasing */
    FLASH->CR |= FLASH_CR_STRT;
    /* Wait till no operation is on going */
    while ((FLASH->SR & FLASH_SR_BSY) != 0) {
        /* Note: we might want a timeout here */
    }
    /* Check the EOP flag in the FLASH_SR register */
    if ((FLASH->SR & FLASH_SR_EOP) != 0) {
        /* Clear EOP flag by software by writing EOP at 1 */
        FLASH->SR = FLASH_SR_EOP;
    } else {
        /* Manage the error cases */
    }
    /* Reset the PER Bit to disable the page erase */
    FLASH->CR &= ~FLASH_CR_PER;
}

void pdbs_config_flash_erase(void)
{
    /* Enter a critical zone */
    chSysLock();

    flash_unlock();

    /* Erase the flash page */
    flash_erase();

    flash_lock();

    /* There is no configuration now, so update config_cur to reflect this */
    config_cur = NULL;

    /* Exit the critical zone */
    chSysUnlock();
}

void pdbs_config_flash_update(const struct pdbs_config *cfg)
{
    /* Enter a critical zone */
    chSysLock();

    flash_unlock();

    /* If there is an old entry, invalidate it. */
    struct pdbs_config *old = pdbs_config_flash_read();
    if (old != NULL) {
        flash_write_halfword(&(old->status), PDBS_CONFIG_STATUS_INVALID);
    }

    /* Find the first empty entry */
    struct pdbs_config *empty = NULL;
    for (int i = 0; i < PDBS_CONFIG_ARRAY_LEN; i++) {
        /* If we've found it, return it. */
        if (pdbs_config_array[i].status == PDBS_CONFIG_STATUS_EMPTY) {
            empty = &pdbs_config_array[i];
            break;
        }
    }
    /* If empty is still NULL, the page is full.  Erase it. */
    if (empty == NULL) {
        flash_erase();
        /* Write to the first element */
        empty = &pdbs_config_array[0];
    }

    /* Write the new configuration */
    flash_write_halfword(&(empty->status), cfg->status);
    flash_write_halfword(&(empty->flags), cfg->flags);
    flash_write_halfword(&(empty->v), cfg->v);
    flash_write_halfword(&(empty->i), cfg->i);
    flash_write_halfword(&(empty->vmin), cfg->vmin);
    flash_write_halfword(&(empty->vmax), cfg->vmax);

    flash_lock();

    /* Update config_cur for the new configuration */
    config_cur = empty;

    /* Exit the critical zone */
    chSysUnlock();
}

struct pdbs_config *pdbs_config_flash_read(void)
{
    /* If we already know where the configuration is, return its location */
    if (config_cur != NULL) {
        return config_cur;
    }

    /* We don't know where the configuration is (config_cur == NULL), so we
     * need to find it and store its location if applicable. */

    /* If the first element is empty, there is no valid structure. */
    if (pdbs_config_array[0].status == PDBS_CONFIG_STATUS_EMPTY) {
        return NULL;
    }

    /* Find the valid structure, if there is one. */
    for (int i = 0; i < PDBS_CONFIG_ARRAY_LEN; i++) {
        /* If we've found it, return it. */
        if (pdbs_config_array[i].status == PDBS_CONFIG_STATUS_VALID) {
            config_cur = &pdbs_config_array[i];
            return config_cur;
        }
    }

    /* If we got to the end, none of the structures is valid. */
    return NULL;
}
