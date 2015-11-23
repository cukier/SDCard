#include <18F25K22.h>
#zero_ram
#fuses HSH, NOPLLEN
#use delay(clock=25M)
#use rs232(baud=9600, UART1)
#use spi(master, mode=0, baud=100000, spi1, force_hw, stream=SPI)

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

#define _SS PIN_A5

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

void mmcsd_deselect(void) {
//	spi_write(0xFF);
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
		packet[5] = 0xFF;

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
		response = spi_read(0xFF);
		if (response != 0xFF)
			return response;
		timeout--;
	}

	return 0xFF;
}

int mmcsd_get_r2(int *r2) {

	r2[1] = mmcsd_get_r1();
	r2[0] = spi_read(0xFF);
	return 0;
}

int mmcsd_get_r7(int *r7) {
	int i;

	r7[4] = mmcsd_get_r1();

	for (i = 0; i < 4; i++)
		r7[3 - i] = spi_read(0xFF);

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

int mmcsd_init(int *r7) {
	int r, i;

	mmcsd_deselect();
	for (i = 0; i < 10; ++i)
		spi_read(0xFF);

	delay_us(300);
	mmcsd_select();
	r = mmcsd_go_idle_state(TRUE);
	mmcsd_deselect();

	delay_us(300);
	mmcsd_select();
	r = mmcsd_send_op_cond(TRUE);
	mmcsd_deselect();

	return r;
}

int main(void) {

	int r7[5] = { 0, 0, 0, 0, 0 };
	int cont, r;

	spi_init(SPI, TRUE);
	printf("\n\rSDCard");
	delay_ms(15);

	r = mmcsd_init(r7);
	if (r != RESP_TIMEOUT) {
		printf("\n\r%u", r);

		for (cont = 0; cont < 5; ++cont)
			printf(" %u", r7[cont]);
	} else
		printf("\n\rTime out!");

	while (TRUE)
		;

	return 0;
}
