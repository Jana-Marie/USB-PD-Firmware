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

#include "protocol_tx.h"

#include "priorities.h"
#include "policy_engine.h"
#include "protocol_rx.h"
#include "fusb302b.h"
#include "messages.h"
#include "pd.h"


thread_t *pdb_prltx_thread;

/* Protocol layer TX thread mailbox */
static msg_t pdb_prltx_mailbox_queue[PDB_MSG_POOL_SIZE];
mailbox_t pdb_prltx_mailbox;

/*
 * Protocol TX machine states
 *
 * Because the PHY can automatically send retries, the Check_RetryCounter state
 * has been removed, transitions relating to it are modified appropriately, and
 * we don't even keep a RetryCounter.
 */
enum protocol_tx_state {
    PRLTxPHYReset,
    PRLTxWaitMessage,
    PRLTxReset,
    PRLTxConstructMessage,
    PRLTxWaitResponse,
    PRLTxMatchMessageID,
    PRLTxTransmissionError,
    PRLTxMessageSent,
    PRLTxDiscardMessage
};

/* The message we're currently working on transmitting */
static union pd_msg *protocol_tx_message = NULL;
/* The ID to be used in transmission */
int8_t pdb_prltx_messageidcounter = 0;


/*
 * PRL_Tx_PHY_Layer_Reset state
 */
static enum protocol_tx_state protocol_tx_phy_reset(struct pdb_config *cfg)
{
    (void) cfg;
    /* Reset the PHY */
    fusb_reset();

    /* If a message was pending when we got here, tell the policy engine that
     * we failed to send it */
    if (protocol_tx_message != NULL) {
        /* Tell the policy engine that we failed */
        chEvtSignal(pdb_pe_thread, PDB_EVT_PE_TX_ERR);
        /* Finish failing to send the message */
        protocol_tx_message = NULL;
    }

    /* Wait for a message request */
    return PRLTxWaitMessage;
}

/*
 * PRL_Tx_Wait_for_Message_Request state
 */
static enum protocol_tx_state protocol_tx_wait_message(struct pdb_config *cfg)
{
    (void) cfg;
    /* Wait for an event */
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PRLTX_RESET | PDB_EVT_PRLTX_DISCARD
            | PDB_EVT_PRLTX_MSG_TX);

    if (evt & PDB_EVT_PRLTX_RESET) {
        return PRLTxPHYReset;
    }
    if (evt & PDB_EVT_PRLTX_DISCARD) {
        return PRLTxDiscardMessage;
    }

    /* If the policy engine is trying to send a message */
    if (evt & PDB_EVT_PRLTX_MSG_TX) {
        /* Get the message */
        chMBFetch(&pdb_prltx_mailbox, (msg_t *) &protocol_tx_message, TIME_IMMEDIATE);
        /* If it's a Soft_Reset, reset the TX layer first */
        if (PD_MSGTYPE_GET(protocol_tx_message) == PD_MSGTYPE_SOFT_RESET
                && PD_NUMOBJ_GET(protocol_tx_message) == 0) {
            return PRLTxReset;
        /* Otherwise, just send the message */
        } else {
            return PRLTxConstructMessage;
        }
    }

    /* Silence the compiler warning */
    return PRLTxDiscardMessage;
}

static enum protocol_tx_state protocol_tx_reset(struct pdb_config *cfg)
{
    /* Clear MessageIDCounter */
    pdb_prltx_messageidcounter = 0;

    /* Tell the Protocol RX thread to reset */
    chEvtSignal(cfg->prl.rx_thread, PDB_EVT_PRLRX_RESET);
    chThdYield();

    return PRLTxConstructMessage;
}

/*
 * PRL_Tx_Construct_Message state
 */
static enum protocol_tx_state protocol_tx_construct_message(struct pdb_config *cfg)
{
    (void) cfg;
    /* Make sure nobody wants us to reset */
    eventmask_t evt = chEvtGetAndClearEvents(PDB_EVT_PRLTX_RESET | PDB_EVT_PRLTX_DISCARD);

    if (evt & PDB_EVT_PRLTX_RESET) {
        return PRLTxPHYReset;
    }
    if (evt & PDB_EVT_PRLTX_DISCARD) {
        return PRLTxDiscardMessage;
    }

    /* Set the correct MessageID in the message */
    protocol_tx_message->hdr &= ~PD_HDR_MESSAGEID;
    protocol_tx_message->hdr |= pdb_prltx_messageidcounter << PD_HDR_MESSAGEID_SHIFT;

    /* Send the message to the PHY */
    fusb_send_message(protocol_tx_message);

    return PRLTxWaitResponse;
}

/*
 * PRL_Tx_Wait_for_PHY_Response state
 */
static enum protocol_tx_state protocol_tx_wait_response(struct pdb_config *cfg)
{
    (void) cfg;
    /* Wait for an event.  There is no need to run CRCReceiveTimer, since the
     * FUSB302B handles that as part of its retry mechanism. */
    eventmask_t evt = chEvtWaitAny(PDB_EVT_PRLTX_RESET | PDB_EVT_PRLTX_DISCARD
            | PDB_EVT_PRLTX_I_TXSENT | PDB_EVT_PRLTX_I_RETRYFAIL);

    if (evt & PDB_EVT_PRLTX_RESET) {
        return PRLTxPHYReset;
    }
    if (evt & PDB_EVT_PRLTX_DISCARD) {
        return PRLTxDiscardMessage;
    }

    /* If the message was sent successfully */
    if (evt & PDB_EVT_PRLTX_I_TXSENT) {
        return PRLTxMatchMessageID;
    }
    /* If the message failed to be sent */
    if (evt & PDB_EVT_PRLTX_I_RETRYFAIL) {
        return PRLTxTransmissionError;
    }

    /* Silence the compiler warning */
    return PRLTxDiscardMessage;
}

/*
 * PRL_Tx_Match_MessageID state
 */
static enum protocol_tx_state protocol_tx_match_messageid(struct pdb_config *cfg)
{
    (void) cfg;
    union pd_msg goodcrc;

    /* Read the GoodCRC */
    fusb_read_message(&goodcrc);

    /* Check that the message is correct */
    if (PD_MSGTYPE_GET(&goodcrc) == PD_MSGTYPE_GOODCRC
            && PD_NUMOBJ_GET(&goodcrc) == 0
            && PD_MESSAGEID_GET(&goodcrc) == pdb_prltx_messageidcounter) {
        return PRLTxMessageSent;
    } else {
        return PRLTxTransmissionError;
    }
}

static enum protocol_tx_state protocol_tx_transmission_error(struct pdb_config *cfg)
{
    (void) cfg;
    /* Increment MessageIDCounter */
    pdb_prltx_messageidcounter = (pdb_prltx_messageidcounter + 1) % 8;

    /* Tell the policy engine that we failed */
    chEvtSignal(pdb_pe_thread, PDB_EVT_PE_TX_ERR);

    protocol_tx_message = NULL;
    return PRLTxWaitMessage;
}

static enum protocol_tx_state protocol_tx_message_sent(struct pdb_config *cfg)
{
    (void) cfg;
    /* Increment MessageIDCounter */
    pdb_prltx_messageidcounter = (pdb_prltx_messageidcounter + 1) % 8;

    /* Tell the policy engine that we succeeded */
    chEvtSignal(pdb_pe_thread, PDB_EVT_PE_TX_DONE);

    protocol_tx_message = NULL;
    return PRLTxWaitMessage;
}

static enum protocol_tx_state protocol_tx_discard_message(struct pdb_config *cfg)
{
    (void) cfg;
    /* If we were working on sending a message, increment MessageIDCounter */
    if (protocol_tx_message != NULL) {
        pdb_prltx_messageidcounter = (pdb_prltx_messageidcounter + 1) % 8;
    }

    return PRLTxPHYReset;
}

/*
 * Protocol layer TX state machine thread
 */
static THD_FUNCTION(ProtocolTX, cfg) {

    enum protocol_tx_state state = PRLTxPHYReset;

    /* Initialize the mailbox */
    chMBObjectInit(&pdb_prltx_mailbox, pdb_prltx_mailbox_queue, PDB_MSG_POOL_SIZE);

    while (true) {
        switch (state) {
            case PRLTxPHYReset:
                state = protocol_tx_phy_reset(cfg);
                break;
            case PRLTxWaitMessage:
                state = protocol_tx_wait_message(cfg);
                break;
            case PRLTxReset:
                state = protocol_tx_reset(cfg);
                break;
            case PRLTxConstructMessage:
                state = protocol_tx_construct_message(cfg);
                break;
            case PRLTxWaitResponse:
                state = protocol_tx_wait_response(cfg);
                break;
            case PRLTxMatchMessageID:
                state = protocol_tx_match_messageid(cfg);
                break;
            case PRLTxTransmissionError:
                state = protocol_tx_transmission_error(cfg);
                break;
            case PRLTxMessageSent:
                state = protocol_tx_message_sent(cfg);
                break;
            case PRLTxDiscardMessage:
                state = protocol_tx_discard_message(cfg);
                break;
            default:
                /* This is an error.  It really shouldn't happen.  We might
                 * want to handle it anyway, though. */
                break;
        }
    }
}

void pdb_prltx_run(struct pdb_config *cfg)
{
    pdb_prltx_thread = chThdCreateStatic(cfg->prl._tx_wa,
            sizeof(cfg->prl._tx_wa), PDB_PRIO_PRL, ProtocolTX, cfg);
}
