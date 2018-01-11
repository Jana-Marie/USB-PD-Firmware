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

#ifndef PDB_CONF_H
#define PDB_CONF_H


/* Number of messages in the message pool */
#define PDB_MSG_POOL_SIZE 4

/* Size of the Policy Engine thread's working area */
#define PDB_PE_WA_SIZE 256

/* Size of the protocol layer RX thread's working area */
#define PDB_PRLRX_WA_SIZE 256

/* Size of the protocol layer TX thread's working area */
#define PDB_PRLTX_WA_SIZE 256

/* Size of the protocol layer hard reset thread's working area */
#define PDB_HARDRST_WA_SIZE 256

/* Size of the INT_N thread's working area */
#define PDB_INT_N_WA_SIZE 128


#endif /* PDB_CONF_H */
