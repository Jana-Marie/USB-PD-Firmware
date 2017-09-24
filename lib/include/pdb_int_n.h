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

#ifndef PDB_INT_N_H
#define PDB_INT_N_H

#include <ch.h>

#include "pdb_conf.h"


/*
 * Structure for the INT_N thread
 */
struct pdb_int_n {
    /* INT_N thread and working area */
    THD_WORKING_AREA(_wa, PDB_INT_N_WA_SIZE);
    thread_t *thread;
};


#endif /* PDB_INT_N_H */
