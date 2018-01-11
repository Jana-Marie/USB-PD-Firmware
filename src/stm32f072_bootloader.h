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

#ifndef PDBS_STM32F072_BOOTLOADER_H
#define PDBS_STM32F072_BOOTLOADER_H

#include <stdint.h>

#include <ch.h>

/*
 * Code to jump to the STM32F072 bootloader, based on
 * https://stackoverflow.com/a/36793779
 */

#define SYSMEM_RESET_VECTOR 0x1FFFC804
#define RESET_TO_BOOTLOADER_MAGIC_CODE 0xDEADBEEF
#define BOOTLOADER_STACK_POINTER 0x20002250


/* Magic value used for jumping to the STM32F072 bootloader */
extern uint32_t dfu_reset_to_bootloader_magic;

/*
 * Reset the microcontroller and run the bootloader
 */
void dfu_run_bootloader(void);


#endif /* PDBS_STM32F072_BOOTLOADER_H */
