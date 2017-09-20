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
#include "config.h"
#include "led.h"
#include "device_policy_manager.h"
#include "policy_engine.h"
#include "pd.h"


/* Buffer for unwritten configuration */
static struct pdbs_config tmpcfg = {
    .status = PDBS_CONFIG_STATUS_VALID
};

/*
 * Helper functions for printing PDOs
 */
static void print_src_fixed_pdo(BaseSequentialStream *chp, uint32_t pdo)
{
    int tmp;

    chprintf(chp, "fixed\r\n");

    /* Dual-role power */
    tmp = (pdo & PD_PDO_SRC_FIXED_DUAL_ROLE_PWR) >> PD_PDO_SRC_FIXED_DUAL_ROLE_PWR_SHIFT;
    if (tmp) {
        chprintf(chp, "\tdual_role_pwr: %d\r\n", tmp);
    }

    /* USB Suspend Supported */
    tmp = (pdo & PD_PDO_SRC_FIXED_USB_SUSPEND) >> PD_PDO_SRC_FIXED_USB_SUSPEND_SHIFT;
    if (tmp) {
        chprintf(chp, "\tusb_suspend: %d\r\n", tmp);
    }

    /* Unconstrained power */
    tmp = (pdo & PD_PDO_SRC_FIXED_UNCONSTRAINED) >> PD_PDO_SRC_FIXED_UNCONSTRAINED_SHIFT;
    if (tmp) {
        chprintf(chp, "\tunconstrained_pwr: %d\r\n", tmp);
    }

    /* USB communications capable */
    tmp = (pdo & PD_PDO_SRC_FIXED_USB_COMMS) >> PD_PDO_SRC_FIXED_USB_COMMS_SHIFT;
    if (tmp) {
        chprintf(chp, "\tusb_comms: %d\r\n", tmp);
    }

    /* Dual-role data */
    tmp = (pdo & PD_PDO_SRC_FIXED_DUAL_ROLE_DATA) >> PD_PDO_SRC_FIXED_DUAL_ROLE_DATA_SHIFT;
    if (tmp) {
        chprintf(chp, "\tdual_role_data: %d\r\n", tmp);
    }

    /* Peak current */
    tmp = (pdo & PD_PDO_SRC_FIXED_PEAK_CURRENT) >> PD_PDO_SRC_FIXED_PEAK_CURRENT_SHIFT;
    if (tmp) {
        chprintf(chp, "\tpeak_i: %d\r\n", tmp);
    }

    /* Voltage */
    tmp = (pdo & PD_PDO_SRC_FIXED_VOLTAGE) >> PD_PDO_SRC_FIXED_VOLTAGE_SHIFT;
    chprintf(chp, "\tv: %d.%02d V\r\n", PD_PDV_V(tmp), PD_PDV_CV(tmp));

    /* Maximum current */
    tmp = (pdo & PD_PDO_SRC_FIXED_CURRENT) >> PD_PDO_SRC_FIXED_CURRENT_SHIFT;
    chprintf(chp, "\ti: %d.%02d A\r\n", PD_PDI_A(tmp), PD_PDI_CA(tmp));
}

static void print_src_pdo(BaseSequentialStream *chp, uint32_t pdo, uint8_t index)
{
    /* If we have a positive index, print a label for the PDO */
    if (index) {
        chprintf(chp, "PDO %d: ", index);
    }

    /* Select the appropriate method for printing the PDO itself */
    if ((pdo & PD_PDO_TYPE) == PD_PDO_TYPE_FIXED) {
        print_src_fixed_pdo(chp, pdo);
    } else {
        /* Unknown PDO, just print it as hex */
        chprintf(chp, "%08X\r\n", pdo);
    }
}


/*
 * Command functions
 */

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

    pdbs_config_flash_erase();
}

static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: write\r\n");
        return;
    }

    pdbs_config_flash_update(&tmpcfg);

    chEvtSignal(pdb_pe_thread, PDB_EVT_PE_NEW_POWER);
}

static void cmd_load(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: load\r\n");
        return;
    }

    /* Get the current configuration */
    struct pdbs_config *cfg = pdbs_config_flash_read();
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
    struct pdbs_config *cfg = NULL;

    if (argc > 1) {
        chprintf(chp, "Usage: get_cfg [index]\r\n");
        return;
    }

    /* With no arguments, find the current configuration */
    if (argc == 0) {
        cfg = pdbs_config_flash_read();
        if (cfg == NULL) {
            chprintf(chp, "No configuration\r\n");
            return;
        }
    /* With an argument, get a particular configuration array index */
    } else if (argc == 1) {
        char *endptr;
        long i = strtol(argv[0], &endptr, 0);
        if (i >= 0 && i < PDBS_CONFIG_ARRAY_LEN && endptr > argv[0]) {
            cfg = &pdbs_config_array[i];
        } else {
            chprintf(chp, "Invalid index\r\n");
            return;
        }
    }
    /* Print the configuration */
    pdbs_config_print(chp, cfg);
}

static void cmd_get_tmpcfg(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: get_tmpcfg\r\n");
        return;
    }

    pdbs_config_print(chp, &tmpcfg);
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
    tmpcfg.flags ^= PDBS_CONFIG_FLAGS_GIVEBACK;
}

static void cmd_set_v(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc != 1) {
        chprintf(chp, "Usage: set_v voltage_in_mV\r\n");
        return;
    }

    char *endptr;
    long i = strtol(argv[0], &endptr, 0);
    if (i >= PD_MV_MIN && i <= PD_MV_MAX && endptr > argv[0]) {
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
    if (i >= PD_MA_MIN && i <= PD_MA_MAX && endptr > argv[0]) {
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

static void cmd_output(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc == 0) {
        /* With no arguments, print the output status */
        if (pdb_dpm_output_enabled) {
            chprintf(chp, "enabled\r\n");
        } else {
            chprintf(chp, "disabled\r\n");
        }
    } else if (argc == 1) {
        /* Set the output status and re-negotiate power */
        if (strcmp(argv[0], "enable") == 0) {
            pdb_dpm_output_enabled = true;
            chEvtSignal(pdb_pe_thread, PDB_EVT_PE_NEW_POWER);
        } else if (strcmp(argv[0], "disable") == 0) {
            pdb_dpm_output_enabled = false;
            chEvtSignal(pdb_pe_thread, PDB_EVT_PE_NEW_POWER);
        } else {
            /* Or, if the argument was invalid, print a usage message */
            chprintf(chp, "Usage: output [enable|disable]\r\n");
        }
    } else {
        /* If there are too many arguments, print a usage message */
        chprintf(chp, "Usage: output [enable|disable]\r\n");
    }
}

static void cmd_get_source_cap(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argv;
    if (argc > 0) {
        chprintf(chp, "Usage: get_source_cap\r\n");
        return;
    }

    /* If we haven't seen any Source_Capabilities */
    if (pdb_dpm_capabilities == NULL) {
        /* Have we started reading Type-C Current advertisements? */
        if (pdb_dpm_typec_current != None) {
            /* Type-C Current is available, so report it */
            chprintf(chp, "PDO 1: typec_virtual\r\n");
            if (pdb_dpm_typec_current == Default) {
                chprintf(chp, "\ti: 0.50 A\r\n");
            } else if (pdb_dpm_typec_current == OnePointFiveAmps) {
                chprintf(chp, "\ti: 1.50 A\r\n");
            } else if (pdb_dpm_typec_current == ThreePointZeroAmps) {
                chprintf(chp, "\ti: 3.00 A\r\n");
            }
            return;
        } else {
            /* No Type-C Current, so report no capabilities */
            chprintf(chp, "No Source_Capabilities\r\n");
            return;
        }
    }

    /* Print all the PDOs */
    uint8_t numobj = PD_NUMOBJ_GET(pdb_dpm_capabilities);
    for (uint8_t i = 0; i < numobj; i++) {
        print_src_pdo(chp, pdb_dpm_capabilities->obj[i], i+1);
    }
}

/*
 * List of shell commands
 */
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
    {"output", cmd_output, "Get or set the output status"},
    {"get_source_cap", cmd_get_source_cap, "Print the capabilities of the PD source"},
    {NULL, NULL, NULL}
};

/*
 * The shell's configuration
 */
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
 * PD Buddy Sink configuration shell
 */
void pdb_shell(void)
{
    int n;
    BaseSequentialStream *chp = shell_cfg.io;
    const struct pdb_shell_cmd *scp = shell_cfg.commands;
    char *lp, *cmd, *tokp, line[PDB_SHELL_MAX_LINE_LENGTH];
    char *args[PDB_SHELL_MAX_ARGUMENTS + 1];

    while (true) {
        /* Print the prompt */
        chprintf(chp, "PDBS) ");

        /* Read a line of input */
        if (shellGetLine(chp, line, sizeof(line))) {
            /* If a command was not entered, prompt again */
            chprintf(chp, "\r\n");
            continue;
        }

        /* Tokenize the line */
        lp = _strtok(line, " \t", &tokp);
        cmd = lp;
        n = 0;
        while ((lp = _strtok(NULL, " \t", &tokp)) != NULL) {
            /* If we have too many tokens, abort */
            if (n >= PDB_SHELL_MAX_ARGUMENTS) {
                chprintf(chp, "too many arguments\r\n");
                cmd = NULL;
                break;
            }
            args[n++] = lp;
        }
        args[n] = NULL;

        /* If there's a command to run, run it */
        if (cmd != NULL) {
            /* Handle "help" in a special way */
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
            /* Run a command, giving a generic error message if there is no
             * such command */
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

        /* Read a character */
        /* The cast to BaseAsynchronousChannel * is safe because we know that
         * chp is always really of that type. */
        while (chnReadTimeout((BaseAsynchronousChannel *) chp, (uint8_t *)&c, 1, TIME_IMMEDIATE) == 0)
            chThdSleepMilliseconds(2);
        /* Abort if ^D is received */
        if (c == '\x04') {
            chprintf(chp, "^D");
            return true;
        }
        /* Delete a character if ASCII backspace or delete is received */
        if ((c == '\b') || (c == '\x7F')) {
            if (p != line) {
                chSequentialStreamPut(chp, 0x08);
                chSequentialStreamPut(chp, 0x20);
                chSequentialStreamPut(chp, 0x08);
                p--;
            }
            continue;
        }
        /* Finish reading input if Enter is pressed */
        if (c == '\r') {
            chprintf(chp, "\r\n");
            *p = 0;
            return false;
        }
        /* Ignore other non-printing characters */
        if (c < ' ')
            continue;
        /* If there's room in the line buffer, append the new character */
        if (p < line + size - 1) {
            chSequentialStreamPut(chp, c);
            *p++ = (char)c;
        }
    }
}
