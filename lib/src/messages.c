/*
 * PD Buddy Firmware Library - USB Power Delivery for everyone
 * Copyright 2017-2018 Clayton G. Hobbs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "messages.h"

#include <pdb_msg.h>

#include "pdb_conf.h"


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
