/*
 * sdcard.c
 *
 *  Created on: 13 de nov de 2015
 *      Author: cuki
 */

enum MMCSD_err {
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

enum _card_type {
	SD, MMC
} g_card_type;

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

#ifndef _SS
#define _SS PIN_A5
#endif

short g_CRC_enabled;
short g_MMCSDBufferChanged;

void sd_select() {
	output_low(MMCSD_PIN_SELECT);
}

void sd_deselect(void) {
	spi_write(0xFF);
	output_high(_SS);
}

int send_cmd(int cmd, long long arg) {
	int packet[6];

	packet[0] = cmd | 0x40;
	packet[1] = make8(arg, 3);
	packet[2] = make8(arg, 2);
	packet[3] = make8(arg, 1);
	packet[4] = make8(arg, 0);
	packet[5] = 0xFF;

	spi_write(packet[0]);
	spi_write(packet[1]);
	spi_write(packet[2]);
	spi_write(packet[3]);
	spi_write(packet[4]);
	spi_write(packet[5]);

	return 0;
}

int sd_get_r(void) {
	int resp, timeout = 0xFF;

	while (timeout) {
		resp = spi_read(0xFF);
		if (resp != 0xFF)
			return resp;
		timeout--;
	}
	return RESP_TIMEOUT;
}

int mmcsd_go_idle_state(void) {
	send_cmd(GO_IDLE_STATE, 0);

	return sd_get_r();
}

int mmcsd_send_op_cond(void) {
	send_cmd(SEND_OP_COND, 0);

	return sd_get_r();
}

int mmcsd_app_cmd(void) {
	send_cmd(APP_CMD, 0);

	return sd_get_r();
}

int mmcsd_sd_send_op_cond(void) {
	send_cmd(SD_SEND_OP_COND, 0);

	return sd_get_r();
}

int mmcsd_set_blocklen(long long blocklen) {
	send_cmd(SET_BLOCKLEN, blocklen);

	return sd_get_r();
}

int mmcsd_crc_on_off(short crc_enabled) {
	send_cmd(CRC_ON_OFF, crc_enabled);

	g_CRC_enabled = crc_enabled;

	return sd_get_r();
}

int mmcsd_read_single_block(long long address) {
	send_cmd(READ_SINGLE_BLOCK, address);

	return sd_get_r();
}

int mmcsd_read_block(long long address, long size, int *ptr) {
	int ec;
	long i; // counter for loops

	// send command
	sd_select();
	ec = mmcsd_read_single_block(address);
	if (ec != MMCSD_GOODEC) {
		sd_deselect();
		return ec;
	}

	// wait for the data start token
	ec = mmcsd_wait_for_token(DATA_START_TOKEN);//<-----
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	// read in the data
	for (i = 0; i < size; i += 1)
		ptr[i] = MMCSD_SPI_XFER(0xFF);

	if (g_CRC_enabled) {
		/* check the crc */
		if (make16(MMCSD_SPI_XFER(0xFF), MMCSD_SPI_XFER(0xFF))
				!= mmcsd_crc16(g_mmcsd_buffer, MMCSD_MAX_BLOCK_SIZE)) {
			mmcsd_deselect();
			return MMCSD_CRC_ERR;
		}
	} else {
		/* have the card transmit the CRC, but ignore it */
		MMCSD_SPI_XFER(0xFF);
		MMCSD_SPI_XFER(0xFF);
	}
	mmcsd_deselect();

	return MMCSD_GOODEC;
}

int mmcsd_load_buffer(void) {
	g_MMCSDBufferChanged = FALSE;
	return (mmcsd_read_block(g_mmcsdBufferAddress, MMCSD_MAX_BLOCK_SIZE,
			g_mmcsd_buffer));
}

int init_sd() {
	int r, i;

	sd_deselect();
	delay_ms(15);

	i = 0;
	do {
		delay_ms(1);
		sd_select();
		r = mmcsd_go_idle_state();
		sd_deselect();
		i++;
		if (i == 0xFF) {
			sd_deselect();
			return r;
		}
	} while (!bit_test(r, 0));

	i = 0;
	do {
		delay_ms(1);
		sd_select();
		r = mmcsd_send_op_cond();
		sd_deselect();
		i++;
		if (i == 0xFF) {
			sd_deselect();
			return r;
		}
	} while (r & MMCSD_IDLE);

	sd_select();
	r = mmcsd_app_cmd();
	r = mmcsd_sd_send_op_cond();
	sd_deselect();

	if (r == 0x04)
		g_card_type = MMC;
	else
		g_card_type = SD;

	sd_select();
	r = mmcsd_set_blocklen(512);
	if (r != MMCSD_GOODEC) {
		sd_deselect();
		return r;
	}
	sd_deselect();
	setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_XMIT_L_TO_H | SPI_CLK_DIV_4);
	sd_select();
	r = mmcsd_crc_on_off(0);
	if (r != MMCSD_GOODEC) {
		sd_deselect();
		return r;
	}
	sd_deselect();

r =

return r;
}
