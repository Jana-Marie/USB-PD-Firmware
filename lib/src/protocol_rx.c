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

#include "protocol_rx.h"

#include <stdlib.h>

#include "priorities.h"
#include "policy_engine.h"
#include "protocol_tx.h"
#include "fusb302b.h"
#include "messages.h"
#include "pd.h"


/*
 * Protocol RX machine states
 *
 * There is no Send_GoodCRC state because the PHY sends the GoodCRC for us.
 * All transitions that would go to that state instead go to Check_MessageID.
 */
enum protocol_rx_state {
    PRLRxWaitPHY,
    PRLRxReset,
    PRLRxCheckMessageID,
    PRLRxStoreMessageID
};

/* The received message we're currently working with */
static union pd_msg *protocol_rx_message = NULL;
/* The ID of the last received message */
int8_t pdb_prlrx_messageid = -1;

/*
 * PRL_Rx_Wait_for_PHY_Message state
 */
static enum protocol_rx_state protocol_rx_wait_phy(struct pdb_config *cfg)
{
    (void) cfg;
    /* Wait for an event */
    eventmask_t evt = chEvtWaitAny(ALL_EVENTS);

    /* If we got a reset event, reset */
    if (evt & PDB_EVT_PRLRX_RESET) {
        return PRLRxWaitPHY;
    }
    /* If we got an I_GCRCSENT event, read the message and decide what to do */
    if (evt & PDB_EVT_PRLRX_I_GCRCSENT) {
        /* Get a buffer to read the message into.  Guaranteed to not fail
         * because we have a big enough pool and are careful. */
        protocol_rx_message = chPoolAlloc(&pdb_msg_pool);
        /* Read the message */
        fusb_read_message(protocol_rx_message);
        /* If it's a Soft_Reset, go to the soft reset state */
        if (PD_MSGTYPE_GET(protocol_rx_message) == PD_MSGTYPE_SOFT_RESET
                && PD_NUMOBJ_GET(protocol_rx_message) == 0) {
            return PRLRxReset;
        /* Otherwise, check the message ID */
        } else {
            return PRLRxCheckMessageID;
        }
    }

    /* We shouldn't ever get here.  This just silence the compiler warning. */
    return PRLRxWaitPHY;
}

/*
 * PRL_Rx_Layer_Reset_for_Receive state
 */
static enum protocol_rx_state protocol_rx_reset(struct pdb_config *cfg)
{
    /* Reset MessageIDCounter */
    pdb_prltx_messageidcounter = 0;

    /* Clear stored MessageID */
    pdb_prlrx_messageid = -1;

    /* TX transitions to its reset state */
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_RESET);
    chThdYield();

    /* If we got a RESET signal, reset the machine */
    if (chEvtGetAndClearEvents(PDB_EVT_PRLRX_RESET) != 0) {
        chPoolFree(&pdb_msg_pool, protocol_rx_message);
        protocol_rx_message = NULL;
        return PRLRxWaitPHY;
    }

    /* Go to the Check_MessageID state */
    return PRLRxCheckMessageID;
}

/*
 * PRL_Rx_Check_MessageID state
 */
static enum protocol_rx_state protocol_rx_check_messageid(struct pdb_config *cfg)
{
    (void) cfg;
    /* If we got a RESET signal, reset the machine */
    if (chEvtGetAndClearEvents(PDB_EVT_PRLRX_RESET) != 0) {
        chPoolFree(&pdb_msg_pool, protocol_rx_message);
        protocol_rx_message = NULL;
        return PRLRxWaitPHY;
    }

    /* If the message has the stored ID, we've seen this message before.  Free
     * it and don't pass it to the policy engine. */
    if (PD_MESSAGEID_GET(protocol_rx_message) == pdb_prlrx_messageid) {
        chPoolFree(&pdb_msg_pool, protocol_rx_message);
        protocol_rx_message = NULL;
        return PRLRxWaitPHY;
    /* Otherwise, there's either no stored ID or this message has an ID we
     * haven't just seen.  Transition to the Store_MessageID state. */
    } else {
        return PRLRxStoreMessageID;
    }
}

/*
 * PRL_Rx_Store_MessageID state
 */
static enum protocol_rx_state protocol_rx_store_messageid(struct pdb_config *cfg)
{
    /* Tell ProtocolTX to discard the message being transmitted */
    chEvtSignal(cfg->prl.tx_thread, PDB_EVT_PRLTX_DISCARD);
    chThdYield();

    /* Update the stored MessageID */
    pdb_prlrx_messageid = PD_MESSAGEID_GET(protocol_rx_message);

    /* Pass the message to the policy engine. */
    chMBPost(&pdb_pe_mailbox, (msg_t) protocol_rx_message, TIME_IMMEDIATE);
    chEvtSignal(pdb_pe_thread, PDB_EVT_PE_MSG_RX);

    /* Don't check if we got a RESET because we'd do nothing different. */

    return PRLRxWaitPHY;
}

/*
 * Protocol layer RX state machine thread
 */
static THD_FUNCTION(ProtocolRX, cfg) {
    enum protocol_rx_state state = PRLRxWaitPHY;

    while (true) {
        switch (state) {
            case PRLRxWaitPHY:
                state = protocol_rx_wait_phy(cfg);
                break;
            case PRLRxReset:
                state = protocol_rx_reset(cfg);
                break;
            case PRLRxCheckMessageID:
                state = protocol_rx_check_messageid(cfg);
                break;
            case PRLRxStoreMessageID:
                state = protocol_rx_store_messageid(cfg);
                break;
            default:
                /* This is an error.  It really shouldn't happen.  We might
                 * want to handle it anyway, though. */
                break;
        }
    }
}

void pdb_prlrx_run(struct pdb_config *cfg)
{
    cfg->prl.rx_thread = chThdCreateStatic(cfg->prl._rx_wa,
            sizeof(cfg->prl._rx_wa), PDB_PRIO_PRL, ProtocolRX, cfg);
}
