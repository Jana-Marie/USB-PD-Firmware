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

#include "policy_engine.h"

#include <stdbool.h>

#include "messages.h"
#include "priorities.h"
#include "protocol_tx.h"
#include "hard_reset.h"
#include "fusb302b.h"
#include "pd.h"


thread_t *pdb_pe_thread;

enum policy_engine_state {
    PESinkStartup,
    PESinkDiscovery,
    PESinkWaitCap,
    PESinkEvalCap,
    PESinkSelectCap,
    PESinkTransitionSink,
    PESinkReady,
    PESinkGetSourceCap,
    PESinkGiveSinkCap,
    PESinkHardReset,
    PESinkTransitionDefault,
    PESinkSoftReset,
    PESinkSendSoftReset,
    PESinkSendReject,
    PESinkSourceUnresponsive
};

/* The received message we're currently working with */
static union pd_msg *policy_engine_message = NULL;
/* The most recent Request from the DPM */
static union pd_msg *last_dpm_request = NULL;
/* Whether or not the source capabilities match our required power */
static bool capability_match = false;
/* Whether or not we have an explicit contract */
static bool explicit_contract = false;
/* Whether or not we're receiving minimum power*/
static bool min_power = false;
/* Keep track of how many hard resets we've sent */
static int hard_reset_counter = 0;
/* Policy Engine thread mailbox */
static msg_t pdb_pe_mailbox_queue[PDB_MSG_POOL_SIZE];
mailbox_t pdb_pe_mailbox;

static enum policy_engine_state pe_sink_startup(struct pdb_config *cfg)
{
    /* We don't have an explicit contract currently */
    explicit_contract = false;
    /* Tell the DPM that we've started negotiations */
    cfg->dpm.pd_start(cfg);

    /* No need to reset the protocol layer here.  There are two ways into this
     * state: startup and exiting hard reset.  On startup, the protocol layer
     * is reset by the startup procedure.  When exiting hard reset, the
     * protocol layer is reset by the hard reset state machine.  Since it's
     * already done somewhere else, there's no need to do it again here. */

    return PESinkDiscovery;
}

static enum policy_engine_state pe_sink_discovery(struct pdb_config *cfg)
{
    /* Wait for VBUS.  Since it's our only power source, we already know that
     * we have it, so just move on. */

    return PESinkWaitCap;
}

static enum policy_engine_state pe_sink_wait_cap(struct pdb_config *cfg)
{
    /* Fetch a message from the protocol layer */
    eventmask_t evt = chEvtWaitAnyTimeout(PDB_EVT_PE_MSG_RX
            | PDB_EVT_PE_I_OVRTEMP | PDB_EVT_PE_RESET, PD_T_TYPEC_SINK_WAIT_CAP);
    /* If we timed out waiting for Source_Capabilities, send a hard reset */
    if (evt == 0) {
        return PESinkHardReset;
    }
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If we're too hot, we shouldn't negotiate power yet */
    if (evt & PDB_EVT_PE_I_OVRTEMP) {
        return PESinkWaitCap;
    }

    /* If we got a message */
    if (evt & PDB_EVT_PE_MSG_RX) {
        /* Get the message */
        if (chMBFetch(&pdb_pe_mailbox, (msg_t *) &policy_engine_message, TIME_IMMEDIATE) == MSG_OK) {
            /* If we got a Source_Capabilities message, read it. */
            if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SOURCE_CAPABILITIES
                    && PD_NUMOBJ_GET(policy_engine_message) > 0) {
                return PESinkEvalCap;
            /* If the message was a Soft_Reset, do the soft reset procedure */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SOFT_RESET
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSoftReset;
            /* If we got an unexpected message, reset */
            } else {
                /* Free the received message */
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                return PESinkHardReset;
            }
        }
    }

    /* If we failed to get a message, send a hard reset */
    return PESinkHardReset;
}

static enum policy_engine_state pe_sink_eval_cap(struct pdb_config *cfg)
{
    /* Get a message object for the request if we don't have one already */
    if (last_dpm_request == NULL) {
        last_dpm_request = chPoolAlloc(&pdb_msg_pool);
    }
    /* Ask the DPM what to request */
    capability_match = cfg->dpm.evaluate_capability(cfg, policy_engine_message,
            last_dpm_request);
    /* It's up to the DPM to free the Source_Capabilities message, which it can
     * do whenever it sees fit.  Just remove our reference to it since we won't
     * know when it's no longer valid. */
    policy_engine_message = NULL;

    return PESinkSelectCap;
}

static enum policy_engine_state pe_sink_select_cap(struct pdb_config *cfg)
{
    /* Transmit the request */
    chMBPost(&pdb_prltx_mailbox, (msg_t) last_dpm_request, TIME_IMMEDIATE);
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_MSG_TX);
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PE_TX_DONE | PDB_EVT_PE_TX_ERR
            | PDB_EVT_PE_RESET);
    /* Don't free the request; we might need it again */
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If the message transmission failed, send a hard reset */
    if ((evt & PDB_EVT_PE_TX_DONE) == 0) {
        return PESinkHardReset;
    }

    /* Wait for a response */
    evt = chEvtWaitAnyTimeout(PDB_EVT_PE_MSG_RX | PDB_EVT_PE_RESET,
            PD_T_SENDER_RESPONSE);
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If we didn't get a response before the timeout, send a hard reset */
    if (evt == 0) {
        return PESinkHardReset;
    }

    /* Get the response message */
    if (chMBFetch(&pdb_pe_mailbox, (msg_t *) &policy_engine_message, TIME_IMMEDIATE) == MSG_OK) {
        /* If the source accepted our request, wait for the new power */
        if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_ACCEPT
                && PD_NUMOBJ_GET(policy_engine_message) == 0) {
            /* Transition to Sink Standby if necessary */
            cfg->dpm.transition_standby(cfg);

            min_power = false;

            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkTransitionSink;
        /* If the message was a Soft_Reset, do the soft reset procedure */
        } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SOFT_RESET
                && PD_NUMOBJ_GET(policy_engine_message) == 0) {
            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkSoftReset;
        /* If the message was Wait or Reject */
        } else if ((PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_REJECT
                    || PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_WAIT)
                && PD_NUMOBJ_GET(policy_engine_message) == 0) {
            /* If we don't have an explicit contract, wait for capabilities */
            if (!explicit_contract) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkWaitCap;
            /* If we do have an explicit contract, go to the ready state */
            } else {
                /* If we got here from a Wait message, we Should run
                 * SinkRequestTimer in the Ready state. */
                min_power = (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_WAIT);

                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkReady;
            }
        } else {
            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkSendSoftReset;
        }
    }
    return PESinkHardReset;
}

static enum policy_engine_state pe_sink_transition_sink(struct pdb_config *cfg)
{
    /* Wait for the PS_RDY message */
    eventmask_t evt = chEvtWaitAnyTimeout(PDB_EVT_PE_MSG_RX | PDB_EVT_PE_RESET,
            PD_T_PS_TRANSITION);
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If no message was received, send a hard reset */
    if (evt == 0) {
        return PESinkHardReset;
    }

    /* If we received a message, read it */
    if (chMBFetch(&pdb_pe_mailbox, (msg_t *) &policy_engine_message, TIME_IMMEDIATE) == MSG_OK) {
        /* If we got a PS_RDY, handle it */
        if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_PS_RDY
                && PD_NUMOBJ_GET(policy_engine_message) == 0) {
            /* We just finished negotiating an explicit contract */
            explicit_contract = true;

            /* Set the output appropriately */
            if (!min_power) {
                cfg->dpm.transition_requested(cfg);
            }

            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkReady;
        /* If there was a protocol error, send a hard reset */
        } else {
            /* Turn off the power output before this hard reset to make sure we
             * don't supply an incorrect voltage to the device we're powering.
             */
            cfg->dpm.transition_default(cfg);

            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkHardReset;
        }
    }

    return PESinkHardReset;
}

static enum policy_engine_state pe_sink_ready(struct pdb_config *cfg)
{
    eventmask_t evt;

    /* Wait for an event */
    if (min_power) {
        evt = chEvtWaitAnyTimeout(PDB_EVT_PE_MSG_RX | PDB_EVT_PE_RESET
                | PDB_EVT_PE_I_OVRTEMP | PDB_EVT_PE_GET_SOURCE_CAP
                | PDB_EVT_PE_NEW_POWER, PD_T_SINK_REQUEST);
    } else {
        evt = chEvtWaitAny(PDB_EVT_PE_MSG_RX | PDB_EVT_PE_RESET
                | PDB_EVT_PE_I_OVRTEMP | PDB_EVT_PE_GET_SOURCE_CAP
                | PDB_EVT_PE_NEW_POWER);
    }

    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }

    /* If we overheated, send a hard reset */
    if (evt & PDB_EVT_PE_I_OVRTEMP) {
        return PESinkHardReset;
    }

    /* If the DPM wants us to, send a Get_Source_Cap message */
    if (evt & PDB_EVT_PE_GET_SOURCE_CAP) {
        return PESinkGetSourceCap;
    }

    /* If the DPM wants new power, let it figure out what power it wants
     * exactly.  This isn't exactly the transition from the spec (that would be
     * SelectCap, not EvalCap), but this works better with the particular
     * design of this firmware. */
    if (evt & PDB_EVT_PE_NEW_POWER) {
        /* Make sure we're evaluating NULL capabilities to use the old ones */
        if (policy_engine_message != NULL) {
            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
        }
        return PESinkEvalCap;
    }

    /* If no event was received, the timer ran out. */
    if (evt == 0) {
        /* Repeat our Request message */
        return PESinkSelectCap;
    }

    /* If we received a message */
    if (evt & PDB_EVT_PE_MSG_RX) {
        if (chMBFetch(&pdb_pe_mailbox, (msg_t *) &policy_engine_message, TIME_IMMEDIATE) == MSG_OK) {
            /* Ignore vendor-defined messages */
            if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_VENDOR_DEFINED
                    && PD_NUMOBJ_GET(policy_engine_message) > 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkReady;
            /* Ignore Ping messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_PING
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkReady;
            /* Reject DR_Swap messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_DR_SWAP
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendReject;
            /* Reject Get_Source_Cap messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_GET_SOURCE_CAP
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendReject;
            /* Reject PR_Swap messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_PR_SWAP
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendReject;
            /* Reject VCONN_Swap messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_VCONN_SWAP
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendReject;
            /* Reject Request messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_REQUEST
                    && PD_NUMOBJ_GET(policy_engine_message) > 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendReject;
            /* Reject Sink_Capabilities messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SINK_CAPABILITIES
                    && PD_NUMOBJ_GET(policy_engine_message) > 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendReject;
            /* Handle GotoMin messages */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_GOTOMIN
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                if (cfg->dpm.giveback_enabled(cfg)) {
                    /* Transition to the minimum current level */
                    cfg->dpm.transition_min(cfg);
                    min_power = true;

                    chPoolFree(&pdb_msg_pool, policy_engine_message);
                    policy_engine_message = NULL;
                    return PESinkTransitionSink;
                } else {
                    /* We don't support GiveBack, so send a Reject */
                    chPoolFree(&pdb_msg_pool, policy_engine_message);
                    policy_engine_message = NULL;
                    return PESinkSendReject;
                }
            /* Evaluate new Source_Capabilities */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SOURCE_CAPABILITIES
                    && PD_NUMOBJ_GET(policy_engine_message) > 0) {
                /* Don't free the message: we need to keep the
                 * Source_Capabilities message so we can evaluate it. */
                return PESinkEvalCap;
            /* Give sink capabilities when asked */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_GET_SINK_CAP
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkGiveSinkCap;
            /* If the message was a Soft_Reset, do the soft reset procedure */
            } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SOFT_RESET
                    && PD_NUMOBJ_GET(policy_engine_message) == 0) {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSoftReset;
            /* If we got an unknown message, send a soft reset */
            } else {
                chPoolFree(&pdb_msg_pool, policy_engine_message);
                policy_engine_message = NULL;
                return PESinkSendSoftReset;
            }
        }
    }

    return PESinkReady;
}

static enum policy_engine_state pe_sink_get_source_cap(struct pdb_config *cfg)
{
    /* Get a message object */
    union pd_msg *get_source_cap = chPoolAlloc(&pdb_msg_pool);
    /* Make a Get_Source_Cap message */
    get_source_cap->hdr = PD_MSGTYPE_GET_SOURCE_CAP | PD_DATAROLE_UFP
        | PD_SPECREV_2_0 | PD_POWERROLE_SINK | PD_NUMOBJ(0);
    /* Transmit the Get_Source_Cap */
    chMBPost(&pdb_prltx_mailbox, (msg_t) get_source_cap, TIME_IMMEDIATE);
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_MSG_TX);
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PE_TX_DONE | PDB_EVT_PE_TX_ERR
            | PDB_EVT_PE_RESET);
    /* Free the sent message */
    chPoolFree(&pdb_msg_pool, get_source_cap);
    get_source_cap = NULL;
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If the message transmission failed, send a hard reset */
    if ((evt & PDB_EVT_PE_TX_DONE) == 0) {
        return PESinkHardReset;
    }

    return PESinkReady;
}

static enum policy_engine_state pe_sink_give_sink_cap(struct pdb_config *cfg)
{
    /* Get a message object */
    union pd_msg *snk_cap = chPoolAlloc(&pdb_msg_pool);
    /* Get our capabilities from the DPM */
    cfg->dpm.get_sink_capability(cfg, snk_cap);

    /* Transmit our capabilities */
    chMBPost(&pdb_prltx_mailbox, (msg_t) snk_cap, TIME_IMMEDIATE);
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_MSG_TX);
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PE_TX_DONE | PDB_EVT_PE_TX_ERR
            | PDB_EVT_PE_RESET);

    /* Free the Sink_Capabilities message */
    chPoolFree(&pdb_msg_pool, snk_cap);
    snk_cap = NULL;

    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If the message transmission failed, send a hard reset */
    if ((evt & PDB_EVT_PE_TX_DONE) == 0) {
        return PESinkHardReset;
    }

    return PESinkReady;
}

static enum policy_engine_state pe_sink_hard_reset(struct pdb_config *cfg)
{
    /* If we've already sent the maximum number of hard resets, assume the
     * source is unresponsive. */
    if (hard_reset_counter > PD_N_HARD_RESET_COUNT) {
        return PESinkSourceUnresponsive;
    }

    /* Generate a hard reset signal */
    chEvtSignal(cfg->prl.hardrst_thread, PDB_EVT_HARDRST_RESET);
    chEvtWaitAny(PDB_EVT_PE_HARD_SENT);

    /* Increment HardResetCounter */
    hard_reset_counter++;

    return PESinkTransitionDefault;
}

static enum policy_engine_state pe_sink_transition_default(struct pdb_config *cfg)
{
    explicit_contract = false;

    /* Tell the DPM to transition to default power */
    cfg->dpm.transition_default(cfg);

    /* There is no local hardware to reset. */
    /* Since we never change our data role from UFP, there is no reason to set
     * it here. */

    /* Tell the protocol layer we're done with the reset */
    chEvtSignal(cfg->prl.hardrst_thread, PDB_EVT_HARDRST_DONE);

    return PESinkStartup;
}

static enum policy_engine_state pe_sink_soft_reset(struct pdb_config *cfg)
{
    /* No need to explicitly reset the protocol layer here.  It resets itself
     * when a Soft_Reset message is received. */

    /* Get a message object */
    union pd_msg *accept = chPoolAlloc(&pdb_msg_pool);
    /* Make an Accept message */
    accept->hdr = PD_MSGTYPE_ACCEPT | PD_DATAROLE_UFP | PD_SPECREV_2_0
        | PD_POWERROLE_SINK | PD_NUMOBJ(0);
    /* Transmit the Accept */
    chMBPost(&pdb_prltx_mailbox, (msg_t) accept, TIME_IMMEDIATE);
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_MSG_TX);
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PE_TX_DONE | PDB_EVT_PE_TX_ERR
            | PDB_EVT_PE_RESET);
    /* Free the sent message */
    chPoolFree(&pdb_msg_pool, accept);
    accept = NULL;
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If the message transmission failed, send a hard reset */
    if ((evt & PDB_EVT_PE_TX_DONE) == 0) {
        return PESinkHardReset;
    }

    return PESinkWaitCap;
}

static enum policy_engine_state pe_sink_send_soft_reset(struct pdb_config *cfg)
{
    /* No need to explicitly reset the protocol layer here.  It resets itself
     * just before a Soft_Reset message is transmitted. */

    /* Get a message object */
    union pd_msg *softrst = chPoolAlloc(&pdb_msg_pool);
    /* Make a Soft_Reset message */
    softrst->hdr = PD_MSGTYPE_SOFT_RESET | PD_DATAROLE_UFP | PD_SPECREV_2_0
        | PD_POWERROLE_SINK | PD_NUMOBJ(0);
    /* Transmit the soft reset */
    chMBPost(&pdb_prltx_mailbox, (msg_t) softrst, TIME_IMMEDIATE);
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_MSG_TX);
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PE_TX_DONE | PDB_EVT_PE_TX_ERR
            | PDB_EVT_PE_RESET);
    /* Free the sent message */
    chPoolFree(&pdb_msg_pool, softrst);
    softrst = NULL;
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If the message transmission failed, send a hard reset */
    if ((evt & PDB_EVT_PE_TX_DONE) == 0) {
        return PESinkHardReset;
    }

    /* Wait for a response */
    evt = chEvtWaitAnyTimeout(PDB_EVT_PE_MSG_RX | PDB_EVT_PE_RESET,
            PD_T_SENDER_RESPONSE);
    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If we didn't get a response before the timeout, send a hard reset */
    if (evt == 0) {
        return PESinkHardReset;
    }

    /* Get the response message */
    if (chMBFetch(&pdb_pe_mailbox, (msg_t *) &policy_engine_message, TIME_IMMEDIATE) == MSG_OK) {
        /* If the source accepted our soft reset, wait for capabilities. */
        if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_ACCEPT
                && PD_NUMOBJ_GET(policy_engine_message) == 0) {
            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkWaitCap;
        /* If the message was a Soft_Reset, do the soft reset procedure */
        } else if (PD_MSGTYPE_GET(policy_engine_message) == PD_MSGTYPE_SOFT_RESET
                && PD_NUMOBJ_GET(policy_engine_message) == 0) {
            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkSoftReset;
        /* Otherwise, send a hard reset */
        } else {
            chPoolFree(&pdb_msg_pool, policy_engine_message);
            policy_engine_message = NULL;
            return PESinkHardReset;
        }
    }
    return PESinkHardReset;
}

static enum policy_engine_state pe_sink_send_reject(struct pdb_config *cfg)
{
    /* Get a message object */
    union pd_msg *reject = chPoolAlloc(&pdb_msg_pool);
    /* Make a Reject message */
    reject->hdr = PD_MSGTYPE_REJECT | PD_DATAROLE_UFP | PD_SPECREV_2_0
        | PD_POWERROLE_SINK | PD_NUMOBJ(0);

    /* Transmit the message */
    chMBPost(&pdb_prltx_mailbox, (msg_t) reject, TIME_IMMEDIATE);
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_MSG_TX);
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PE_TX_DONE | PDB_EVT_PE_TX_ERR
            | PDB_EVT_PE_RESET);

    /* Free the Reject message */
    chPoolFree(&pdb_msg_pool, reject);
    reject = NULL;

    /* If we got reset signaling, transition to default */
    if (evt & PDB_EVT_PE_RESET) {
        return PESinkTransitionDefault;
    }
    /* If the message transmission failed, send a soft reset */
    if ((evt & PDB_EVT_PE_TX_DONE) == 0) {
        return PESinkSendSoftReset;
    }

    return PESinkReady;
}

/*
 * When Power Delivery is unresponsive, fall back to Type-C Current
 */
static enum policy_engine_state pe_sink_source_unresponsive(struct pdb_config *cfg)
{
    static int old_tcc_match = -1;
    int tcc_match = cfg->dpm.evaluate_typec_current(cfg, fusb_get_typec_current());

    /* If the last two readings are the same, set the output */
    if (old_tcc_match == tcc_match) {
        cfg->dpm.transition_typec(cfg);
    }

    /* Remember whether or not the last measurement succeeded */
    old_tcc_match = tcc_match;

    /* Wait tPDDebounce between measurements */
    chThdSleep(PD_T_PD_DEBOUNCE);

    return PESinkSourceUnresponsive;
}

/*
 * Policy Engine state machine thread
 */
static THD_FUNCTION(PolicyEngine, cfg) {
    enum policy_engine_state state = PESinkStartup;

    /* Initialize the mailbox */
    chMBObjectInit(&pdb_pe_mailbox, pdb_pe_mailbox_queue, PDB_MSG_POOL_SIZE);

    while (true) {
        switch (state) {
            case PESinkStartup:
                state = pe_sink_startup(cfg);
                break;
            case PESinkDiscovery:
                state = pe_sink_discovery(cfg);
                break;
            case PESinkWaitCap:
                state = pe_sink_wait_cap(cfg);
                break;
            case PESinkEvalCap:
                state = pe_sink_eval_cap(cfg);
                break;
            case PESinkSelectCap:
                state = pe_sink_select_cap(cfg);
                break;
            case PESinkTransitionSink:
                state = pe_sink_transition_sink(cfg);
                break;
            case PESinkReady:
                state = pe_sink_ready(cfg);
                break;
            case PESinkGetSourceCap:
                state = pe_sink_get_source_cap(cfg);
                break;
            case PESinkGiveSinkCap:
                state = pe_sink_give_sink_cap(cfg);
                break;
            case PESinkHardReset:
                state = pe_sink_hard_reset(cfg);
                break;
            case PESinkTransitionDefault:
                state = pe_sink_transition_default(cfg);
                break;
            case PESinkSoftReset:
                state = pe_sink_soft_reset(cfg);
                break;
            case PESinkSendSoftReset:
                state = pe_sink_send_soft_reset(cfg);
                break;
            case PESinkSendReject:
                state = pe_sink_send_reject(cfg);
                break;
            case PESinkSourceUnresponsive:
                state = pe_sink_source_unresponsive(cfg);
                break;
            default:
                /* This is an error.  It really shouldn't happen.  We might
                 * want to handle it anyway, though. */
                state = PESinkStartup;
                break;
        }
    }
}

void pdb_pe_run(struct pdb_config *cfg)
{
    pdb_pe_thread = chThdCreateStatic(cfg->pe._wa, sizeof(cfg->pe._wa),
            PDB_PRIO_PE, PolicyEngine, cfg);
}
