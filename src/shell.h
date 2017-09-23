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

/*
   ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef PDB_SHELL_H
#define PDB_SHELL_H

#include <ch.h>

#include <pdb.h>

#define PDB_SHELL_MAX_LINE_LENGTH 64
#define PDB_SHELL_MAX_ARGUMENTS 2


/* Structure for PD Buddy shell commands */
struct pdbs_shell_cmd {
    char *cmd;
    void (*func)(BaseSequentialStream *chp, int argc, char *argv[]);
    char *desc;
};

/* Structure for PD Buddy shell configuration */
struct pdbs_shell_cfg {
    BaseSequentialStream *io;
    const struct pdbs_shell_cmd *commands;
};


void pdbs_shell(struct pdb_config *cfg);

bool shellGetLine(BaseSequentialStream *chp, char *line, unsigned size);


#endif /* PDB_SHELL_H */
