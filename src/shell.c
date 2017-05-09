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

#include "shell.h"

#include <stdlib.h>
#include <string.h>

#include "chprintf.h"

#include "usbcfg.h"
#include "storage.h"
#include "led.h"
#include "pd.h"


/* Buffer for unwritten configuration */
static struct pdb_config tmpcfg = {
    .status = PDB_CONFIG_STATUS_VALID
};

static void cmd_license(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: license\r\n");
        return;
    }

    chprintf(chp,
        "PD Buddy - USB Power Delivery for everyone\r\n"
        "Copyright (C) 2017 Clayton G. Hobbs <clay@lakeserv.net>\r\n"
        "\r\n"
        "This program is free software: you can redistribute it and/or modify\r\n"
        "it under the terms of the GNU General Public License as published by\r\n"
        "the Free Software Foundation, either version 3 of the License, or\r\n"
        "(at your option) any later version.\r\n"
        "\r\n"
        "This program is distributed in the hope that it will be useful,\r\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n"
        "GNU General Public License for more details.\r\n"
        "\r\n"
        "You should have received a copy of the GNU General Public License\r\n"
        "along with this program.  If not, see <http://www.gnu.org/licenses/>.\r\n"
    );
}

static void cmd_erase(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: erase\r\n");
        return;
    }

    pdb_config_flash_erase();
}

static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: write\r\n");
        return;
    }

    pdb_config_flash_update(&tmpcfg);
}

static void cmd_load(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: load\r\n");
        return;
    }

    /* Get the current configuration */
    struct pdb_config *cfg = pdb_config_flash_read();
    if (cfg == NULL) {
        chprintf(chp, "No configuration\r\n");
        return;
    }

    /* Load the current configuration into tmpcfg */
    tmpcfg.status = cfg->status;
    tmpcfg.flags = cfg->flags;
    tmpcfg.v = cfg->v;
    tmpcfg.i = cfg->i;
    tmpcfg.v_min = cfg->v_min;
    tmpcfg.v_max = cfg->v_max;
}

static void cmd_get_cfg(BaseSequentialStream *chp, int argc, char *argv[])
{
    struct pdb_config *cfg = NULL;

    if (argc > 1) {
        chprintf(chp, "Usage: get_cfg [index]\r\n");
        return;
    }

    /* With no arguments, find the current configuration */
    if (argc == 0) {
        cfg = pdb_config_flash_read();
        if (cfg == NULL) {
            chprintf(chp, "No configuration\r\n");
            return;
        }
    /* With an argument, get a particular configuration array index */
    } else if (argc == 1) {
        char *endptr;
        long i = strtol(argv[0], &endptr, 0);
        if (i >= 0 && i < PDB_CONFIG_ARRAY_LEN && endptr > argv[0]) {
            cfg = &pdb_config_array[i];
        } else {
            chprintf(chp, "Invalid index\r\n");
            return;
        }
    }
    /* Print the configuration */
    pdb_config_print(chp, cfg);
}

static void cmd_get_tmpcfg(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: get_tmpcfg\r\n");
        return;
    }

    pdb_config_print(chp, &tmpcfg);
}

static void cmd_clear_flags(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: clear_flags\r\n");
        return;
    }

    /* Clear all flags */
    tmpcfg.flags = 0;
}

static void cmd_toggle_giveback(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: toggle_giveback\r\n");
        return;
    }

    /* Toggle the GiveBack flag */
    tmpcfg.flags ^= PDB_CONFIG_FLAGS_GIVEBACK;
}

static void cmd_set_v(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc != 1) {
        chprintf(chp, "Usage: set_v voltage_in_mV\r\n");
        return;
    }

    char *endptr;
    long i = strtol(argv[0], &endptr, 0);
    if (i >= 0 && i <= UINT16_MAX && endptr > argv[0]) {
        /* Convert mV to the unit used by USB PD */
        tmpcfg.v = PD_MV2PDV(i);
    } else {
        chprintf(chp, "Invalid voltage\r\n");
        return;
    }
}

static void cmd_set_i(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc != 1) {
        chprintf(chp, "Usage: set_i current_in_mA\r\n");
        return;
    }

    char *endptr;
    long i = strtol(argv[0], &endptr, 0);
    if (i >= 0 && i <= UINT16_MAX && endptr > argv[0]) {
        /* Convert mA to the unit used by USB PD */
        tmpcfg.i = PD_MA2PDI(i);
    } else {
        chprintf(chp, "Invalid current\r\n");
        return;
    }
}

static void cmd_identify(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) chp;
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: identify\r\n");
        return;
    }

    chEvtSignal(pdb_led_thread, PDB_EVT_LED_IDENTIFY);
}

static const struct pdb_shell_cmd commands[] = {
    {"license", cmd_license, "Show copyright and license information"},
    {"erase", cmd_erase, "Erase all stored configuration"},
    {"write", cmd_write, "Store the configuration buffer"},
    {"load", cmd_load, "Load the stored configuration into the buffer"},
    {"get_cfg", cmd_get_cfg, "Print the stored configuration"},
    {"get_tmpcfg", cmd_get_tmpcfg, "Print the configuration buffer"},
    {"clear_flags", cmd_clear_flags, "Clear all flags"},
    {"toggle_giveback", cmd_toggle_giveback, "Toggle the GiveBack flag"},
    /* TODO {"toggle_var_bat", cmd_toggle_var_bat, "Toggle the Var/Bat flag"},*/
    {"set_v", cmd_set_v, "Set the voltage in millivolts"},
    {"set_i", cmd_set_i, "Set the current in milliamps"},
    /* TODO {"set_v_range", cmd_set_v_range, "Set the minimum and maximum voltage in millivolts"},*/
    {"identify", cmd_identify, "Blink the LED to identify the device"},
    {NULL, NULL, NULL}
};

const struct pdb_shell_cfg shell_cfg = {
    (BaseSequentialStream *)&SDU1,
    commands
};

/*
 * Utility functions for the shell
 */
static char *_strtok(char *str, const char *delim, char **saveptr)
{
    char *token;
    if (str)
        *saveptr = str;
    token = *saveptr;

    if (!token)
        return NULL;

    token += strspn(token, delim);
    *saveptr = strpbrk(token, delim);
    if (*saveptr)
        *(*saveptr)++ = '\0';

    return *token ? token : NULL;
}

static void list_commands(BaseSequentialStream *chp, const struct pdb_shell_cmd *scp)
{
    while (scp->cmd != NULL) {
        chprintf(chp, "\t%s: %s\r\n", scp->cmd, scp->desc);
        scp++;
    }
}

static bool cmdexec(const struct pdb_shell_cmd *scp, BaseSequentialStream *chp,
        char *name, int argc, char *argv[])
{
    while (scp->cmd != NULL) {
        if (strcmp(scp->cmd, name) == 0) {
            scp->func(chp, argc, argv);
            return false;
        }
        scp++;
    }
    return true;
}

/*
 * PD Buddy configuration shell
 *
 * p: The configuration for the shell itself
 */
void pdb_shell(void)
{
    int n;
    BaseSequentialStream *chp = shell_cfg.io;
    const struct pdb_shell_cmd *scp = shell_cfg.commands;
    char *lp, *cmd, *tokp, line[PDB_SHELL_MAX_LINE_LENGTH];
    char *args[PDB_SHELL_MAX_ARGUMENTS + 1];

    while (true) {
        chprintf(chp, "PDBS) ");
        if (shellGetLine(chp, line, sizeof(line))) {
            chprintf(chp, "\r\n");
            continue;
        }
        lp = _strtok(line, " \t", &tokp);
        cmd = lp;
        n = 0;
        while ((lp = _strtok(NULL, " \t", &tokp)) != NULL) {
            if (n >= PDB_SHELL_MAX_ARGUMENTS) {
                chprintf(chp, "too many arguments\r\n");
                cmd = NULL;
                break;
            }
            args[n++] = lp;
        }
        args[n] = NULL;
        if (cmd != NULL) {
            if (strcmp(cmd, "help") == 0) {
                if (n > 0) {
                    chprintf(chp, "Usage: help\r\n");
                    continue;
                }
                chprintf(chp, "PD Buddy Sink configuration shell\r\n");
                chprintf(chp, "Commands:\r\n");
                chprintf(chp, "\thelp: Print this message\r\n");
                if (scp != NULL)
                    list_commands(chp, scp);
            }
            else if ((scp == NULL) || cmdexec(scp, chp, cmd, n, args)) {
                chprintf(chp, "%s", cmd);
                chprintf(chp, " ?\r\n");
            }
        }
    }
}


/**
 * @brief   Reads a whole line from the input channel.
 *
 * @param[in] chp       pointer to a @p BaseSequentialStream object
 * @param[in] line      pointer to the line buffer
 * @param[in] size      buffer maximum length
 * @return              The operation status.
 * @retval true         the channel was reset or CTRL-D pressed.
 * @retval false        operation successful.
 *
 * @api
 */
bool shellGetLine(BaseSequentialStream *chp, char *line, unsigned size)
{
    char *p = line;

    while (true) {
        char c;

        if (chSequentialStreamRead(chp, (uint8_t *)&c, 1) == 0)
            return true;
        if (c == 4) {
            chprintf(chp, "^D");
            return true;
        }
        if ((c == 8) || (c == 127)) {
            if (p != line) {
                chSequentialStreamPut(chp, 0x08);
                chSequentialStreamPut(chp, 0x20);
                chSequentialStreamPut(chp, 0x08);
                p--;
            }
            continue;
        }
        if (c == '\r') {
            chprintf(chp, "\r\n");
            *p = 0;
            return false;
        }
        if (c < 0x20)
            continue;
        if (p < line + size - 1) {
            chSequentialStreamPut(chp, c);
            *p++ = (char)c;
        }
    }
}
