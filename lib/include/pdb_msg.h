/*
 * PD Buddy - USB Power Delivery for everyone
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

#ifndef PDB_MSG_H
#define PDB_MSG_H

#include <stdint.h>

#include <ch.h>


/*
 * PD message union
 *
 * This can be safely read from or written to in any form without any
 * transformations because everything in the system is little-endian.
 *
 * Two bytes of padding are required at the start to prevent problems due to
 * alignment.  Specifically, without the padding, &obj[0] != &bytes[2], making
 * the statement in the previous paragraph invalid.
 */
union pd_msg {
    struct {
        uint8_t _pad1[2];
        uint8_t bytes[30];
    } __attribute__((packed));
    struct {
        uint8_t _pad2[2];
        uint16_t hdr;
        union {
            uint32_t obj[7];
            struct {
                uint16_t exthdr;
                uint8_t data[26];
            };
        };
    } __attribute__((packed));
};

/*
 * The pool of messages used by the library
 */
extern memory_pool_t pdb_msg_pool;


#endif /* PDB_MSG_H */
