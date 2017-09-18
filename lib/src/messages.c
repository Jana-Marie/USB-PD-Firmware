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

#include "messages.h"


/* The messages that will be available for threads to pass each other */
static union pd_msg pd_messages[PDB_MSG_POOL_SIZE] __attribute__((aligned(sizeof(stkalign_t))));

/* The pool of available messages */
memory_pool_t pdb_msg_pool;


void pdb_msg_pool_init(void)
{
    /* Initialize the pool itself */
    chPoolObjectInit(&pdb_msg_pool, sizeof (union pd_msg), NULL);

    /* Fill the pool with the available buffers */
    chPoolLoadArray(&pdb_msg_pool, pd_messages, PDB_MSG_POOL_SIZE);
}
