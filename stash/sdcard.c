/*
 * sdcard.c
 *
 *  Created on: 13 de nov de 2015
 *      Author: cuki
 */

#ifndef MMCSD_PIN_SELECT
#define MMCSD_PIN_SELECT PIN_A5
#endif

#define GO_IDLE_STATE 0
#define SEND_OP_COND 1
#define SEND_IF_COND 8
#define SEND_CSD 9
#define SEND_CID 10
#define SD_STATUS 13
#define SEND_STATUS 13
#define SET_BLOCKLEN 16
#define READ_SINGLE_BLOCK 17
#define WRITE_BLOCK 24
#define SD_SEND_OP_COND 41
#define APP_CMD 55
#define READ_OCR 58
#define CRC_ON_OFF 59

#define IDLE_TOKEN 0x01
#define DATA_START_TOKEN 0xFE

#define MMCSD_MAX_BLOCK_SIZE 512

enum MMCSD_err_e {
	MMCSD_GOODEC = 0,
	MMCSD_IDLE = 0x01,
	MMCSD_ERASE_RESET = 0x02,
	MMCSD_ILLEGAL_CMD = 0x04,
	MMCSD_CRC_ERR = 0x08,
	MMCSD_ERASE_SEQ_ERR = 0x10,
	MMCSD_ADDR_ERR = 0x20,
	MMCSD_PARAM_ERR = 0x40,
	RESP_TIMEOUT = 0x80
};

typedef enum MMCSD_err_e MMCSD_err;

short g_CRC_enabled;
short g_MMCSDBufferChanged;
int g_mmcsd_buffer[MMCSD_MAX_BLOCK_SIZE];
long long g_mmcsdBufferAddress;

enum _card_type {
	SD, MMC
} g_card_type;

int mmcsd_crc7(int *data, int length) {
	int i, ibit, c, crc;

	crc = 0x00;                                             // Set initial value

	for (i = 0; i < length; i++, data++) {
		c = *data;

		for (ibit = 0; ibit < 8; ibit++) {
			crc = crc << 1;
			if ((c ^ crc) & 0x80)
				crc = crc ^ 0x09;       // ^ is XOR
			c = c << 1;
		}

		crc = crc & 0x7F;
	}

	shift_left(&crc, 1, 1); // MMC card stores the result in the top 7 bits so shift them left 1
							// Should shift in a 1 not a 0 as one of the cards I have won't work otherwise
	return crc;
}

MMCSD_err mmcsd_send_cmd(int cmd, long long arg) {
	int packet[6]; // the entire command, argument, and crc in one variable

	// construct the packet
	// every command on an SD card is or'ed with 0x40
	packet[0] = cmd | 0x40;
	packet[1] = make8(arg, 3);
	packet[2] = make8(arg, 2);
	packet[3] = make8(arg, 1);
	packet[4] = make8(arg, 0);

	// calculate the crc if needed
	if (g_CRC_enabled)
		packet[5] = mmcsd_crc7(packet, 5);
	else
		packet[5] = 0xFF;

	// transfer the command and argument, with an extra 0xFF hacked in there
	spi_write(packet[0]);
	spi_write(packet[1]);
	spi_write(packet[2]);
	spi_write(packet[3]);
	spi_write(packet[4]);
	spi_write(packet[5]);

	return MMCSD_GOODEC;
}

MMCSD_err mmcsd_get_r1(void) {
	int response = 0, // place to hold the response coming back from the SPI line
			timeout = 0xFF; // maximum amount loops to wait for idle before getting impatient and leaving the function with an error code

	// loop until timeout == 0
	while (timeout) {
		// read what's on the SPI line
		//  the SD/MMC requires that you leave the line high when you're waiting for data from it
		response = spi_read(0xFF);
		//response = MMCSD_SPI_XFER(0x00);//leave the line idle

		// check to see if we got a response
		if (response != 0xFF) {
			// fill in the response that we got and leave the function
			return response;
		}

		// wait for a little bit longer
		timeout--;
	}

	// for some reason, we didn't get a response back from the card
	//  return the proper error codes
	return RESP_TIMEOUT;
}

MMCSD_err mmcsd_read_single_block(long long address) {
	mmcsd_send_cmd(READ_SINGLE_BLOCK, address);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_wait_for_token(int token) {
	MMCSD_err r1;

	// get a token
	r1 = mmcsd_get_r1();

	// check to see if the token we recieved was the one that we were looking for
	if (r1 == token)
		return MMCSD_GOODEC;

	// if that wasn't right, return the error
	return r1;
}

long mmcsd_crc16(char *data, int length) {
	int i, ibit, c;

	long crc;

	crc = 0x0000;                                           // Set initial value

	for (i = 0; i < length; i++, data++) {
		c = *data;

		for (ibit = 0; ibit < 8; ibit++) {
			crc = crc << 1;
			if ((c ^ crc) & 0x8000)
				crc = crc ^ 0x1021;                              // ^ is XOR
			c = c << 1;
		}

		crc = crc & 0x7FFF;
	}

	shift_left(&crc, 2, 1); // MMC card stores the result in the top 7 bits so shift them left 1
							// Should shift in a 1 not a 0 as one of the cards I have won't work otherwise
	return crc;
}

void mmcsd_deselect() {
	spi_write(0xFF);
	output_high(MMCSD_PIN_SELECT);
}

void mmcsd_select() {
	output_low(MMCSD_PIN_SELECT);
}

MMCSD_err mmcsd_read_block(long long address, long size, int* ptr) {
	MMCSD_err ec;
	long i; // counter for loops

	// send command
	mmcsd_select();
	ec = mmcsd_read_single_block(address);
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	// wait for the data start token
	ec = mmcsd_wait_for_token(DATA_START_TOKEN);
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	// read in the data
	for (i = 0; i < size; i += 1)
		ptr[i] = spi_read(0xFF);

	if (g_CRC_enabled) {
		/* check the crc */
		if (make16(spi_read(0xFF), spi_read(0xFF))
				!= mmcsd_crc16(g_mmcsd_buffer, MMCSD_MAX_BLOCK_SIZE)) {
			mmcsd_deselect();
			return MMCSD_CRC_ERR;
		}
	} else {
		/* have the card transmit the CRC, but ignore it */
		spi_write(0xFF);
		spi_write(0xFF);
	}
	mmcsd_deselect();

	return MMCSD_GOODEC;
}

MMCSD_err mmcsd_go_idle_state(void) {
	mmcsd_send_cmd(GO_IDLE_STATE, 0);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_send_op_cond(void) {
	mmcsd_send_cmd(SEND_OP_COND, 0);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_app_cmd(void) {
	mmcsd_send_cmd(APP_CMD, 0);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_sd_send_op_cond(void) {
	mmcsd_send_cmd(SD_SEND_OP_COND, 0);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_set_blocklen(long long blocklen) {
	mmcsd_send_cmd(SET_BLOCKLEN, blocklen);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_crc_on_off(short crc_enabled) {
	mmcsd_send_cmd(CRC_ON_OFF, crc_enabled);

	g_CRC_enabled = crc_enabled;

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_load_buffer(void) {
	g_MMCSDBufferChanged = FALSE;
	return (mmcsd_read_block(g_mmcsdBufferAddress, MMCSD_MAX_BLOCK_SIZE,
			g_mmcsd_buffer));
}

MMCSD_err sd_init() {
	int i, r1;

	setup_spi(SPI_MASTER | SPI_CLK_DIV_64 | SPI_L_TO_H);
	delay_ms(100);

	g_CRC_enabled = TRUE;
	g_mmcsdBufferAddress = 0;

	mmcsd_deselect();
	delay_ms(15);

	i = 0;
	do {
		delay_ms(1);
		mmcsd_select();
		r1 = mmcsd_go_idle_state();
		mmcsd_deselect();
		i++;
		if (i == 0xFF) {
			mmcsd_deselect();
			return r1;
		}
	} while (!bit_test(r1, 0));

	i = 0;
	do {
		delay_ms(1);
		mmcsd_select();
		r1 = mmcsd_send_op_cond();
		mmcsd_deselect();
		i++;
		if (i == 0xFF) {
			mmcsd_deselect();
			return r1;
		}
	} while (r1 & MMCSD_IDLE);

	/* figure out if we have an SD or MMC */
	mmcsd_select();
	r1 = mmcsd_app_cmd();
	r1 = mmcsd_sd_send_op_cond();
	mmcsd_deselect();

	/* an mmc will return an 0x04 here */
	if (r1 == 0x04)
		g_card_type = MMC;
	else
		g_card_type = SD;

	/* set block length to 512 bytes */
	mmcsd_select();
	r1 = mmcsd_set_blocklen(MMCSD_MAX_BLOCK_SIZE);
	if (r1 != MMCSD_GOODEC) {
		mmcsd_deselect();
		return r1;
	}
	mmcsd_deselect();

	/* turn CRCs off to speed up reading/writing */
	mmcsd_select();
	r1 = mmcsd_crc_on_off(0);
	if (r1 != MMCSD_GOODEC) {
		mmcsd_deselect();
		return r1;
	}
	mmcsd_deselect();

	r1 = mmcsd_load_buffer();

	return r1;
}

