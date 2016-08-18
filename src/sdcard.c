/*
 * sdcard.c
 *
 *  Created on: 27 de abr de 2016
 *      Author: cuki
 */

#include "sdcard.h"

void mmcsd_deselect(void) {
	spi_write(DUMMY_BYTE);
	output_high(_SS);
}

void mmcsd_select(void) {

	output_low(_SS);
}

int mmcsd_crc7(int *data, int length) {
	int i, ibit, c, crc;

	crc = 0x00;

	for (i = 0; i < length; i++, data++) {
		c = *data;

		for (ibit = 0; ibit < 8; ibit++) {
			crc = crc << 1;
			if ((c ^ crc) & 0x80)
				crc = crc ^ 0x09;
			c = c << 1;
		}

		crc = crc & 0x7F;
	}

	shift_left(&crc, 1, 1);

	return crc;
}

int mmcsd_send_cmd(int cmd, long long arg, short g_CRC_enabled) {
	int packet[6];

	packet[0] = cmd | 0x40;
	packet[1] = make8(arg, 3);
	packet[2] = make8(arg, 2);
	packet[3] = make8(arg, 1);
	packet[4] = make8(arg, 0);

	if (g_CRC_enabled)
		packet[5] = mmcsd_crc7(packet, 5);
	else
		packet[5] = DUMMY_BYTE;

	spi_write(packet[0]);
	spi_write(packet[1]);
	spi_write(packet[2]);
	spi_write(packet[3]);
	spi_write(packet[4]);
	spi_write(packet[5]);

	return 0;
}

int mmcsd_get_r1(void) {
	int response = 0, timeout = 0xFF;

	while (timeout) {
		response = spi_read(DUMMY_BYTE);
		if (response != DUMMY_BYTE)
			return response;
		timeout--;
	}

	return 0xFF;
}

int mmcsd_get_r2(int *r2) {

	r2[1] = mmcsd_get_r1();
	r2[0] = spi_read(DUMMY_BYTE);
	return 0;
}

int mmcsd_get_r7(int *r7) {
	int i;

	r7[4] = mmcsd_get_r1();

	for (i = 0; i < 4; i++)
		r7[3 - i] = spi_read(DUMMY_BYTE);

	return r7[4];
}

int mmcsd_get_r3(int *r3) {
	return mmcsd_get_r7(r3);
}

int mmcsd_go_idle_state(short crc_check) {

	mmcsd_send_cmd(GO_IDLE_STATE, 0, crc_check);
	return mmcsd_get_r1();
}

int mmcsd_send_op_cond(short crc_check) {

	mmcsd_send_cmd(SEND_OP_COND, 0, crc_check);
	return mmcsd_get_r1();
}

int mmcsd_app_cmd(short crc_check) {

	mmcsd_send_cmd(APP_CMD, 0, crc_check);
	return mmcsd_get_r1();
}

int mmcsd_send_if_cond(short crc_check) {

	mmcsd_send_cmd(SEND_IF_COND, 0, crc_check);
	return mmcsd_get_r1();
}

int mmcsd_sd_send_op_cond(short crc_check) {

	mmcsd_send_cmd(SD_SEND_OP_COND, 0, crc_check);
	return mmcsd_get_r1();
}

int mmcsd_sd_send_cmd(int cmd, long long arg, short crc_check, int *r7) {

	mmcsd_send_cmd(cmd, arg, crc_check);
	return mmcsd_get_r7(r7);
}

int mmcsd_app_send_op_cond(long long arg, short crc_check) {

	mmcsd_app_cmd(crc_check);
	mmcsd_send_cmd(41, arg, crc_check);
	return mmcsd_get_r1();
}

int mmcsd_read_ocr(short crc_check, int *r3) {

	mmcsd_send_cmd(READ_OCR, 0, crc_check);
	return mmcsd_get_r3(r3);
}

int mmcsd_init(int *r7) {
	int r, i;

	mmcsd_deselect();
	for (i = 0; i < 10; ++i)
		spi_read(DUMMY_BYTE);

	delay_ms(10);
	mmcsd_select();
	r = mmcsd_go_idle_state(TRUE);
	mmcsd_deselect();

	if (r != 0x01)
		return 0xFF;

	i = 0;
	do {
		delay_ms(10);
		mmcsd_select();
		r = mmcsd_sd_send_cmd(SEND_IF_COND, 0x1AA, TRUE, r7);
		mmcsd_deselect();
		i++;
		if (i == 0xFF)
			return RESP_TIMEOUT;
	} while (make16(r7[1], r7[0]) != 0x1AA);

	i = 0;
	do {
		delay_ms(10);
		mmcsd_select();
		r = mmcsd_app_send_op_cond(0x40000000, TRUE);
		mmcsd_deselect();
		i++;
		if (i == 0xFF)
			return RESP_TIMEOUT;
	} while (r != 0);

	i = 0;
	do {
		delay_ms(10);
		mmcsd_select();
		mmcsd_send_cmd(CRC_ON_OFF, 0, TRUE);
		r = mmcsd_get_r7(r7);
		mmcsd_deselect();
		i++;
		if (i == 0xFF)
			return RESP_TIMEOUT;
	} while (bit_test(r, 0));

	return r;
}

int mmcsd_read_single_block(long long address) {
	mmcsd_send_cmd(READ_SINGLE_BLOCK, address, FALSE);
	return mmcsd_get_r1();
}

int mmcsd_set_blocken(long long address, long size) {

	mmcsd_send_cmd(SET_BLOCKLEN, size, FALSE);
	return mmcsd_get_r1();
}

int mmcsd_read_block(long long address, int *data, long size) {
	int r1, data_token;
	long i;

	mmcsd_select();
	r1 = mmcsd_read_single_block(address);
	if (r1 != 0)
		return r1;

	do {
		data_token = spi_read(DUMMY_BYTE);
	} while (data_token != DATA_START_TOKEN);

	for (i = 0; i < size; ++i) {
		data[i] = spi_read(DUMMY_BYTE);
	}

	spi_write(DUMMY_BYTE);
	spi_write(DUMMY_BYTE); //CRC
	mmcsd_deselect();

	return r1;
}

int mmcsd_write_single_block(long long address) {

	mmcsd_send_cmd(WRITE_BLOCK, address, FALSE);
	return mmcsd_get_r1();
}

int mmcsd_write_block(long long address, long size, int *data, int *err) {
	int r1;
	long i;

	mmcsd_select();
	r1 = 0xFF;
	r1 = mmcsd_write_single_block(address);

	if (r1) {
		mmcsd_deselect();
		*err = 1;
		return r1;
	}

	spi_write(DUMMY_BYTE);
	spi_write(DATA_START_TOKEN);

	for (i = 0; i < MMCSD_MAX_BLOCK_SIZE; ++i) {
		if (size) {
			spi_write(data[i]);
			size--;
		} else
			spi_write(0);
	}

	r1 = 0;
	spi_write(DUMMY_BYTE);
	spi_write(DUMMY_BYTE);
	r1 = mmcsd_get_r1();

	if (r1 & 0x0A) {
		*err = 2;
		return r1;
	}

	while (!input(MMC_DI))
		spi_write(DUMMY_BYTE);

	mmcsd_deselect();

	return 0;
}

int mmcsd_test(void) {

	long cont;
	int r7[5] = { 0 };
	int r;

	spi_init(SPI, TRUE);
	delay_ms(15);

	r = mmcsd_init(r7);
	if (r != RESP_TIMEOUT && r != 0xFF) {
		return 0;
	} else {
		return 1;
	}
}