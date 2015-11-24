#include <18F25K22.h>
#zero_ram
#fuses HSH, NOPLLEN
#use delay(clock=25M)
#use rs232(baud=9600, UART1)
#use spi(master, mode=0, baud=400000, spi1, force_hw, stream=SPI)

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

int g_mmcsd_buffer[MMCSD_MAX_BLOCK_SIZE];
long long g_mmcsdBufferAddress;
short g_MMCSDBufferChanged;

void mmcsd_deselect(void) {
	spi_write(0xFF);
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
		spi_read(0xFF);

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

int mmcsd_read_single_block(long long address, short g_CRC_enabled) {
	mmcsd_send_cmd(READ_SINGLE_BLOCK, address, g_CRC_enabled);

	return mmcsd_get_r1();
}

int mmcsd_wait_for_token(int token) {
	int r1;

	r1 = mmcsd_get_r1();

	if (r1 == token)
		return MMCSD_GOODEC;

	return r1;
}

long mmcsd_crc16(int *data, int length) {
	int i, ibit, c;
	long crc;

	crc = 0x0000;

	for (i = 0; i < length; i++, data++) {
		c = *data;
		for (ibit = 0; ibit < 8; ibit++) {
			crc = crc << 1;
			if ((c ^ crc) & 0x8000)
				crc = crc ^ 0x1021;
			c = c << 1;
		}
		crc = crc & 0x7FFF;
	}
	shift_left(&crc, 2, 1);
	return crc;
}

int mmcsd_read_block(long long address, long size, int *ptr,
		short g_CRC_enabled) {
	int ec;
	long i;

	mmcsd_select();
	ec = mmcsd_read_single_block(address, g_CRC_enabled);
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	ec = mmcsd_wait_for_token(DATA_START_TOKEN);
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	for (i = 0; i < size; i += 1)
		ptr[i] = spi_read(0xFF);

	if (g_CRC_enabled) {
		if (make16(spi_read(0xFF), spi_read(0xFF))
				!= mmcsd_crc16(g_mmcsd_buffer, MMCSD_MAX_BLOCK_SIZE)) {
			mmcsd_deselect();
			return MMCSD_CRC_ERR;
		}
	} else {
		spi_write(0xFF);
		spi_write(0xFF);
	}
	mmcsd_deselect();

	return MMCSD_GOODEC;
}

int mmcsd_write_single_block(long long address, short g_CRC_enabled) {
	mmcsd_send_cmd(WRITE_BLOCK, address, g_CRC_enabled);

	return mmcsd_get_r1();
}

int mmcsd_write_block(long long address, long size, int *ptr,
		short g_CRC_enabled) {
	int ec;
	long i;

	mmcsd_select();
	ec = mmcsd_write_single_block(address, g_CRC_enabled);
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	spi_write(DATA_START_TOKEN);

	for (i = 0; i < size; i += 1) {
		spi_write(ptr[i]);
	}

	if (g_CRC_enabled)
		spi_write(mmcsd_crc16(ptr, size));
	else {
		spi_write(0xFF);
		spi_write(0xFF);
	}

	ec = mmcsd_get_r1();
	if (ec & 0x0A) {
		mmcsd_deselect();
		return ec;
	}

	while (spi_read(0xFF) == 0)
		;
	mmcsd_deselect();

	return MMCSD_GOODEC;
}

int mmcsd_flush_buffer(short g_CRC_enabled) {
	if (g_MMCSDBufferChanged) {
		g_MMCSDBufferChanged = FALSE;
		return (mmcsd_write_block(g_mmcsdBufferAddress, MMCSD_MAX_BLOCK_SIZE,
				g_mmcsd_buffer, g_CRC_enabled));
	}
	return (0);
}

int mmcsd_load_buffer(short g_CRC_enabled) {
	g_MMCSDBufferChanged = FALSE;
	return (mmcsd_read_block(g_mmcsdBufferAddress, MMCSD_MAX_BLOCK_SIZE,
			g_mmcsd_buffer, g_CRC_enabled));
}

int mmcsd_move_buffer(long long new_addr, short g_CRC_enabled) {
	int ec = MMCSD_GOODEC;
	long long new_block;

	new_block = new_addr - (new_addr % MMCSD_MAX_BLOCK_SIZE);

	if (g_mmcsdBufferAddress != new_block) {
		if (g_MMCSDBufferChanged) {
			ec = mmcsd_flush_buffer(g_CRC_enabled);
			if (ec != MMCSD_GOODEC)
				return ec;
			g_MMCSDBufferChanged = FALSE;
		}
		g_mmcsdBufferAddress = new_block;
		ec = mmcsd_load_buffer(g_CRC_enabled);
	}

	return ec;
}

int mmcsd_write_byte(long long addr, int data, short g_CRC_enabled) {
	int ec;
	ec = mmcsd_move_buffer(addr, g_CRC_enabled);
	if (ec != MMCSD_GOODEC)
		return ec;

	g_mmcsd_buffer[addr % MMCSD_MAX_BLOCK_SIZE] = data;

	g_MMCSDBufferChanged = TRUE;

	return MMCSD_GOODEC;
}

int mmcsd_write_data(long long address, long size, int *ptr,
		short g_CRC_enabled) {
	int ec;
	long i;

	for (i = 0; i < size; i++) {
		ec = mmcsd_write_byte(address++, *ptr++, g_CRC_enabled);
		if (ec != MMCSD_GOODEC)
			return ec;
	}

	return MMCSD_GOODEC;
}

int main(void) {

	int r7[5] = { 0, 0, 0, 0, 0 };
	int cont, r;
	int data[30];

	spi_init(SPI, TRUE);
	printf("\n\rSDCard");
	delay_ms(15);

	r = mmcsd_init(r7);
	if (r != RESP_TIMEOUT && r != 0xFF) {
		printf("\n\r%u === ", r);
		for (cont = 0; cont < 5; ++cont)
			printf(" %x", r7[4 - cont]);
		printf("\n\rCartão inicializdo com sucesso!!");
		printf("\n\rTeste de Leirura");
		r = mmcsd_read_block(0, 25, data, false);
		printf("\t0x%X\t", r);
		if (r == 0) {
			printf(" Sucesso!\n\r");
			for (cont = 0; cont < 25; ++cont)
				printf(" 0x%X", data[cont]);
		} else
			printf(" Falha na Leitura");
		printf("\n\rTeste de Escrita de 1 byte");
		r = mmcsd_write_byte(0, 0x25, false);
		printf("\t0x%X\t", r);
		r = mmcsd_flush_buffer(FALSE);
		printf("\t0x%X\t", r);
		if (r == 0) {
			printf(" Sucesso!\n\r");
			mmcsd_read_block(0, 25, data, false);
			for (cont = 0; cont < 25; ++cont)
				printf(" 0x%X", data[cont]);
		} else
			printf(" Falha na Escrita");
	} else
		printf("\n\rCartão não presente ou sem resposta do cartão!");

	while (TRUE)
		;

	return 0;
}
