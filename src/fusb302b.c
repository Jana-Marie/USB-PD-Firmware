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

#include "fusb302b.h"

#include <ch.h>
#include <hal.h>

#include "pd.h"


/*
 * Read a single byte from the FUSB302B
 */
static uint8_t fusb_read_byte(uint8_t addr)
{
    uint8_t buf;
    i2cMasterTransmit(&I2CD2, FUSB302B_ADDR, &addr, 1, &buf, 1);
    return buf;
}

/*
 * Read multiple bytes from the FUSB302B
 */
static void fusb_read_buf(uint8_t addr, uint8_t size, uint8_t *buf)
{
    i2cMasterTransmit(&I2CD2, FUSB302B_ADDR, &addr, 1, buf, size);
}

/*
 * Write a single byte to the FUSB302B
 */
static void fusb_write_byte(uint8_t addr, uint8_t byte)
{
    uint8_t buf[2] = {addr, byte};
    i2cMasterTransmit(&I2CD2, FUSB302B_ADDR, buf, 2, NULL, 0);
}

/*
 * Write multiple bytes to the FUSB302B
 */
static void fusb_write_buf(uint8_t addr, uint8_t size, const uint8_t *buf)
{
    uint8_t txbuf[size + 1];

    /* Prepare the transmit buffer */
    txbuf[0] = addr;
    for (int i = 0; i < size; i++) {
        txbuf[i + 1] = buf[i];
    }

    i2cMasterTransmit(&I2CD2, FUSB302B_ADDR, txbuf, size + 1, NULL, 0);
}

void fusb_send_message(const union pd_msg *msg)
{
    /* Token sequences for the FUSB302B */
    static uint8_t sop_seq[5] = {0x12, 0x12, 0x12, 0x13, 0x80};
    static uint8_t eop_seq[4] = {0xFF, 0x14, 0xFE, 0xA1};

    /* Take the I2C2 mutex now so there can't be a race condition on sop_seq */
    i2cAcquireBus(&I2CD2);

    /* Get the length of the message: a two-octet header plus NUMOBJ four-octet
     * data objects */
    uint8_t msg_len = 2 + 4 * PD_NUMOBJ_GET(msg);

    /* Set the number of bytes to be transmitted in the packet */
    sop_seq[4] = 0x80 | msg_len;

    /* Write all three parts of the message to the TX FIFO */
    fusb_write_buf(FUSB_FIFOS, 5, sop_seq);
    fusb_write_buf(FUSB_FIFOS, msg_len, msg->bytes);
    fusb_write_buf(FUSB_FIFOS, 4, eop_seq);

    i2cReleaseBus(&I2CD2);
}

uint8_t fusb_read_message(union pd_msg *msg)
{
    uint8_t garbage[4];
    uint8_t numobj;

    i2cAcquireBus(&I2CD2);

    /* If this isn't an SOP message, return error.
     * Because of our configuration, we should be able to assume this means the
     * buffer is empty, and not try to read past a non-SOP message. */
    if ((fusb_read_byte(FUSB_FIFOS) & FUSB_FIFO_RX_TOKEN_BITS)
            != FUSB_FIFO_RX_SOP) {
        i2cReleaseBus(&I2CD2);
        return 1;
    }
    /* Read the message header into msg */
    fusb_read_buf(FUSB_FIFOS, 2, msg->bytes);
    /* Get the number of data objects */
    numobj = PD_NUMOBJ_GET(msg);
    /* If there is at least one data object, read the data objects */
    if (numobj > 0) {
        fusb_read_buf(FUSB_FIFOS, numobj * 4, msg->bytes + 2);
    }
    /* Throw the CRC32 in the garbage, since the PHY already checked it. */
    fusb_read_buf(FUSB_FIFOS, 4, garbage);

    i2cReleaseBus(&I2CD2);
    return 0;
}

void fusb_send_hardrst(void)
{
    i2cAcquireBus(&I2CD2);

    /* Send a hard reset */
    fusb_write_byte(FUSB_CONTROL3, 0x07 | FUSB_CONTROL3_SEND_HARD_RESET);

    i2cReleaseBus(&I2CD2);
}

void fusb_setup(void)
{
    i2cAcquireBus(&I2CD2);

    /* Fully reset the FUSB302B */
    fusb_write_byte(FUSB_RESET, FUSB_RESET_SW_RES);

    /* Turn on all power */
    fusb_write_byte(FUSB_POWER, 0x0F);

    /* Set interrupt masks */
    fusb_write_byte(FUSB_MASK1, 0x00);
    fusb_write_byte(FUSB_MASKA, 0x00);
    fusb_write_byte(FUSB_MASKB, 0x00);
    fusb_write_byte(FUSB_CONTROL0, 0x04);

    /* Enable automatic retransmission */
    fusb_write_byte(FUSB_CONTROL3, 0x07);

    /* Flush the RX buffer */
    fusb_write_byte(FUSB_CONTROL1, FUSB_CONTROL1_RX_FLUSH);

    /* Measure CC1 */
    fusb_write_byte(FUSB_SWITCHES0, 0x07);
    chThdSleepMicroseconds(250);
    uint8_t cc1;
    cc1 = fusb_read_byte(FUSB_STATUS0) & FUSB_STATUS0_BC_LVL;

    /* Measure CC2 */
    fusb_write_byte(FUSB_SWITCHES0, 0x0B);
    chThdSleepMicroseconds(250);
    uint8_t cc2;
    cc2 = fusb_read_byte(FUSB_STATUS0) & FUSB_STATUS0_BC_LVL;

    /* Select the correct CC line for BMC signaling; also enable AUTO_CRC */
    if (cc1 > cc2) {
        fusb_write_byte(FUSB_SWITCHES1, 0x25);
        fusb_write_byte(FUSB_SWITCHES0, 0x07);
    } else {
        fusb_write_byte(FUSB_SWITCHES1, 0x26);
        fusb_write_byte(FUSB_SWITCHES0, 0x0B);
    }

    /* Reset the PD logic */
    fusb_write_byte(FUSB_RESET, FUSB_RESET_PD_RESET);

    i2cReleaseBus(&I2CD2);
}

void fusb_get_status(union fusb_status *status)
{
    i2cAcquireBus(&I2CD2);

    /* Read the interrupt and status flags into status */
    fusb_read_buf(FUSB_STATUS0A, 7, status->bytes);

    i2cReleaseBus(&I2CD2);
}

void fusb_reset(void)
{
    i2cAcquireBus(&I2CD2);

    /* Flush the TX buffer */
    fusb_write_byte(FUSB_CONTROL0, 0x44);
    /* Flush the RX buffer */
    fusb_write_byte(FUSB_CONTROL1, FUSB_CONTROL1_RX_FLUSH);
    /* Reset the PD logic */
    fusb_write_byte(FUSB_RESET, FUSB_RESET_PD_RESET);

    i2cReleaseBus(&I2CD2);
}