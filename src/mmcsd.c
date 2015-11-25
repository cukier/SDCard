/////////////////////////////////////////////////////////////////////////
////                           MMCSD.c                               ////
////                                                                 ////
////    This is a low-level driver for MMC and SD cards.             ////
////                                                                 ////
//// --User Functions--                                              ////
////                                                                 ////
//// mmcsd_init(): Initializes the media.                            ////
////                                                                 ////
//// mmcsd_read_byte(a, p)                                           ////
////  Reads a byte from the MMC/SD card at location a, saves to      ////
////  pointer p.  Returns 0 if OK, non-zero if error.                ////
////                                                                 ////
//// mmcsd_read_data(a, n, p)                                        ////
////  Reads n bytes of data from the MMC/SD card starting at address ////
////  a, saves result to pointer p.  Returns 0 if OK, non-zero if    ////
////  error.                                                         ////
////                                                                 ////
//// mmcsd_flush_buffer()                                            ////
////  The two user write functions (mmcsd_write_byte() and           ////
////  mmcsd_write_data()) maintain a buffer to speed up the writing  ////
////  process.  Whenever a read or write is performed, the write     ////
////  buffer is loaded with the specified page and only the          ////
////  contents of this buffer is changed.  If any future writes      ////
////  cross a page boundary then the buffer in RAM is written        ////
////  to the MMC/SD and then the next page is loaded into the        ////
////  buffer.  mmcsd_flush_buffer() forces the contents in RAM       ////
////  to the MMC/SD card.  Returns 0 if OK, non-zero if errror.      ////
////                                                                 ////
//// mmcsd_write_byte(a, d)                                          ////
////  Writes data byte d to the MMC/SD address a.  Intelligently     ////
////  manages a write buffer, therefore you may need to call         ////
////  mmcsd_flush_buffer() to flush the buffer.                      ////
////                                                                 ////
//// mmcsd_write_data(a, n, p)                                       ////
////  Writes n bytes of data from pointer p to the MMC/SD card       ////
////  starting at address a.  This function intelligently manages    ////
////  a write buffer, therefore if you may need to call              ////
////  mmcsd_flush_buffer() to flush any buffered characters.         ////
////  returns 0 if OK, non-zero if error.                            ////
////                                                                 ////
//// mmcsd_read_block(a, s, p)                                       ////
////  Reads an entire page from the SD/MMC.  Keep in mind that the   ////
////  start of the read has to be aligned to a block                 ////
////  (Address % 512 = 0).  Therefore s must be evenly divisible by  ////
////  512.  At the application level it is much more effecient to    ////
////  to use mmcsd_read_data() or mmcsd_read_byte().  Returns 0      ////
////  if successful, non-zero if error.                              ////
////                                                                 ////
//// mmcsd_write_block(a, s, p):                                     ////
////  Writes an entire page to the SD/MMC.  This will write an       ////
////  entire page to the SD/MMC, so the address and size must be     ////
////  evenly  divisble by 512.  At the application level it is much  ////
////  more effecient to use mmcsd_write_data() or mmcsd_write_byte().////
////  Returns 0 if successful, non-zero if error.                    ////
////                                                                 ////
//// mmcsd_print_cid(): Displays all data in the Card Identification ////
////                     Register. Note this only works on SD cards. ////
////                                                                 ////
//// mmcsd_print_csd(): Displays all data in the Card Specific Data  ////
////                     Register. Note this only works on SD cards. ////
////                                                                 ////
////                                                                 ////
//// --Non-User Functions--                                          ////
////                                                                 ////
//// mmcsd_go_idle_state(): Sends the GO_IDLE_STATE command to the   ////
////                        SD/MMC.                                  ////
//// mmcsd_send_op_cond(): Sends the SEND_OP_COND command to the     ////
////                        SD. Note this command only works on SD.  ////
//// mmcsd_send_if_cond(): Sends the SEND_IF_COND command to the     ////
////                        SD. Note this command only works on SD.  ////
//// mmcsd_sd_status(): Sends the SD_STATUS command to the SD. Note  ////
////                     This command only works on SD cards.        ////
//// mmcsd_send_status(): Sends the SEND_STATUS command to the       ////
////                       SD/MMC.                                   ////
//// mmcsd_set_blocklen(): Sends the SET_BLOCKLEN command along with ////
////                        the desired block length.                ////
//// mmcsd_app_cmd(): Sends the APP_CMD command to the SD. This only ////
////                   works on SD cards and is used just before any ////
////                   SD-only command (e.g. send_op_cond()).        ////
//// mmcsd_read_ocr(): Sends the READ_OCR command to the SD/MMC.     ////
//// mmcsd_crc_on_off(): Sends the CRC_ON_OFF command to the SD/MMC  ////
////                      along with a bit to turn the CRC on/off.   ////
//// mmcsd_send_cmd(): Sends a command and argument to the SD/MMC.   ////
//// mmcsd_get_r1(): Waits for an R1 response from the SD/MMC and    ////
////                  then saves the response to a buffer.           ////
//// mmcsd_get_r2(): Waits for an R2 response from the SD/MMC and    ////
////                  then saves the response to a buffer.           ////
//// mmcsd_get_r3(): Waits for an R3 response from the SD/MMC and    ////
////                  then saves the response to a buffer.           ////
//// mmcsd_get_r7(): Waits for an R7 response from the SD/MMC and    ////
////                  then saves the response to a buffer.           ////
//// mmcsd_wait_for_token(): Waits for a specified token from the    ////
////                          SD/MMC.                                ////
//// mmcsd_crc7(): Generates a CRC7 using a pointer to some data,    ////
////                and how many bytes long the data is.             ////
//// mmcsd_crc16(): Generates a CRC16 using a pointer to some data,  ////
////                and how many bytes long the data is.             ////
////                                                                 ////
/////////////////////////////////////////////////////////////////////////
////        (C) Copyright 2007 Custom Computer Services              ////
//// This source code may only be used by licensed users of the CCS  ////
//// C compiler.  This source code may only be distributed to other  ////
//// licensed users of the CCS C compiler.  No other use,            ////
//// reproduction or distribution is permitted without written       ////
//// permission.  Derivative programs created using this software    ////
//// in object code form are not restricted in any way.              ////
/////////////////////////////////////////////////////////////////////////

#ifndef MMCSD_C
#define MMCSD_C

/////////////////////
////             ////
//// User Config ////
////             ////
/////////////////////

#include <stdint.h>

////////////////////////
////                ////
//// Useful Defines ////
////                ////
////////////////////////

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

////////////////////////
///                  ///
/// Global Variables ///
///                  ///
////////////////////////

uint8_t g_mmcsd_buffer[MMCSD_MAX_BLOCK_SIZE];

int1 g_CRC_enabled;
int1 g_MMCSDBufferChanged;

uint32_t g_mmcsdBufferAddress;

enum _card_type {
	SD, MMC
} g_card_type;

/////////////////////////////
////                     ////
//// Function Prototypes ////
////                     ////
/////////////////////////////

int mmcsd_app_send_op_cond(long long arg);
int mmcsd_sd_send_cmd(int cmd, long long arg, int *r7);
MMCSD_err mmcsd_init();
MMCSD_err mmcsd_read_data(uint32_t address, uint16_t size, uint8_t* ptr);
MMCSD_err mmcsd_read_block(uint32_t address, uint16_t size, uint8_t* ptr);
MMCSD_err mmcsd_write_data(uint32_t address, uint16_t size, uint8_t* ptr);
MMCSD_err mmcsd_write_block(uint32_t address, uint16_t size, uint8_t* ptr);
MMCSD_err mmcsd_go_idle_state(void);
MMCSD_err mmcsd_send_op_cond(void);
MMCSD_err mmcsd_send_if_cond(uint8_t r7[]);
MMCSD_err mmcsd_print_csd();
MMCSD_err mmcsd_print_cid();
MMCSD_err mmcsd_sd_status(uint8_t r2[]);
MMCSD_err mmcsd_send_status(uint8_t r2[]);
MMCSD_err mmcsd_set_blocklen(uint32_t blocklen);
MMCSD_err mmcsd_read_single_block(uint32_t address);
MMCSD_err mmcsd_write_single_block(uint32_t address);
MMCSD_err mmcsd_sd_send_op_cond(void);
MMCSD_err mmcsd_app_cmd(void);
MMCSD_err mmcsd_read_ocr(uint8_t* r1);
MMCSD_err mmcsd_crc_on_off(int1 crc_enabled);
MMCSD_err mmcsd_send_cmd(uint8_t cmd, uint32_t arg);
MMCSD_err mmcsd_get_r1(void);
MMCSD_err mmcsd_get_r2(uint8_t r2[]);
MMCSD_err mmcsd_get_r3(uint8_t r3[]);
MMCSD_err mmcsd_get_r7(uint8_t r7[]);
MMCSD_err mmcsd_wait_for_token(uint8_t token);
uint8_t mmcsd_crc7(char *data, uint8_t length);
uint16_t mmcsd_crc16(char *data, uint8_t length);
void mmcsd_select();
void mmcsd_deselect();

/// Fast Functions ! ///

MMCSD_err mmcsd_load_buffer(void);
MMCSD_err mmcsd_flush_buffer(void);
MMCSD_err mmcsd_move_buffer(uint32_t new_addr);
MMCSD_err mmcsd_read_byte(uint32_t addr, char* data);
MMCSD_err mmcsd_write_byte(uint32_t addr, char data);

//////////////////////////////////
////                          ////
//// Function Implementations ////
////                          ////
//////////////////////////////////

int mmcsd_app_send_op_cond(long long arg) {

	mmcsd_app_cmd();
	mmcsd_send_cmd(41, arg);
	return mmcsd_get_r1();
}

int mmcsd_sd_send_cmd(int cmd, long long arg, int *r7) {

	mmcsd_send_cmd(cmd, arg);
	return mmcsd_get_r7(r7);
}

MMCSD_err mmcsd_init() {
	int r, i;
	int r7[5];

	g_CRC_enabled = TRUE;

	mmcsd_deselect();
	for (i = 0; i < 10; ++i)
		spi_read(0xFF);

	delay_ms(10);
	mmcsd_select();
	r = mmcsd_go_idle_state();
	mmcsd_deselect();

	if (r != 0x01)
		return 0xFF;

	i = 0;
	do {
		delay_ms(10);
		mmcsd_select();
		r = mmcsd_sd_send_cmd(SEND_IF_COND, 0x1AA, r7);
		mmcsd_deselect();
		i++;
		if (i == 0xFF)
			return RESP_TIMEOUT;
	} while (make16(r7[1], r7[0]) != 0x1AA);

	i = 0;
	do {
		delay_ms(10);
		mmcsd_select();
		r = mmcsd_app_send_op_cond(0x40000000);
		mmcsd_deselect();
		i++;
		if (i == 0xFF)
			return RESP_TIMEOUT;
	} while (r != 0);

	i = 0;
	do {
		delay_ms(10);
		mmcsd_select();
		mmcsd_send_cmd(CRC_ON_OFF, 0);
		r = mmcsd_get_r7(r7);
		mmcsd_deselect();
		i++;
		if (i == 0xFF)
			return RESP_TIMEOUT;
	} while (bit_test(r, 0));

	g_CRC_enabled = FALSE;

	return r;
}

MMCSD_err mmcsd_read_data(uint32_t address, uint16_t size, uint8_t* ptr) {
	MMCSD_err r1;
	uint16_t i;  // counter for loops

	for (i = 0; i < size; i++) {
		r1 = mmcsd_read_byte(address++, ptr++);
		if (r1 != MMCSD_GOODEC)
			return r1;
	}

	return MMCSD_GOODEC;
}

MMCSD_err mmcsd_read_block(uint32_t address, uint16_t size, uint8_t* ptr) {
	MMCSD_err ec;
	uint16_t i; // counter for loops

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

MMCSD_err mmcsd_write_data(uint32_t address, uint16_t size, uint8_t* ptr) {
	MMCSD_err ec;
	uint16_t i;  // counter for loops

	for (i = 0; i < size; i++) {
		ec = mmcsd_write_byte(address++, *ptr++);
		if (ec != MMCSD_GOODEC)
			return ec;
	}

	return MMCSD_GOODEC;
}

MMCSD_err mmcsd_write_block(uint32_t address, uint16_t size, uint8_t* ptr) {
	MMCSD_err ec;
	uint16_t i;

	// send command
	mmcsd_select();
	ec = mmcsd_write_single_block(address);
	if (ec != MMCSD_GOODEC) {
		mmcsd_deselect();
		return ec;
	}

	// send a data start token
	spi_write(DATA_START_TOKEN);

	// send all the data
	for (i = 0; i < size; i += 1) {
		spi_write(ptr[i]);
	}

	// if the CRC is enabled we have to calculate it, otherwise just send an 0xFFFF
	if (g_CRC_enabled) {
		spi_write(make8(mmcsd_crc16(ptr, size), 1));
		spi_write(make8(mmcsd_crc16(ptr, size), 0));
	} else {
		spi_write(0xFF);
		spi_write(0xFF);
	}

	// get the error code back from the card; "data accepted" is 0bXXX00101
	ec = mmcsd_get_r1();
	if (ec & 0x0A) {
		mmcsd_deselect();
		return ec;
	}

	// wait for the line to go back high, this indicates that the write is complete
	while (spi_read(0xFF) == 0)
		;
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

MMCSD_err mmcsd_send_if_cond(uint8_t r7[]) {
	mmcsd_send_cmd(SEND_IF_COND, 0x45A);

	return mmcsd_get_r7(r7);
}

MMCSD_err mmcsd_print_csd() {
	uint8_t buf[16], i, r1;

	// MMCs don't support this command
	if (g_card_type == MMC)
		return MMCSD_PARAM_ERR;

	mmcsd_select();
	mmcsd_send_cmd(SEND_CSD, 0);
	r1 = mmcsd_get_r1();
	if (r1 != MMCSD_GOODEC) {
		mmcsd_deselect();
		return r1;
	}

	r1 = mmcsd_wait_for_token(DATA_START_TOKEN);
	if (r1 != MMCSD_GOODEC) {
		mmcsd_deselect();
		return r1;
	}

	for (i = 0; i < 16; i++)
		buf[i] = spi_read(0xFF);
	mmcsd_deselect();
	return r1;
}

MMCSD_err mmcsd_print_cid() {
	uint8_t buf[16], i, r1;

	// MMCs don't support this command
	if (g_card_type == MMC)
		return MMCSD_PARAM_ERR;

	mmcsd_select();
	mmcsd_send_cmd(SEND_CID, 0);
	r1 = mmcsd_get_r1();
	if (r1 != MMCSD_GOODEC) {
		mmcsd_deselect();
		return r1;
	}
	r1 = mmcsd_wait_for_token(DATA_START_TOKEN);
	if (r1 != MMCSD_GOODEC) {
		mmcsd_deselect();
		return r1;
	}

	for (i = 0; i < 16; i++)
		buf[i] = spi_read(0xFF);
	mmcsd_deselect();
	return r1;
}

MMCSD_err mmcsd_sd_status(uint8_t r2[]) {
	uint8_t i;

	mmcsd_select();
	mmcsd_send_cmd(APP_CMD, 0);
	r2[0] = mmcsd_get_r1();
	mmcsd_deselect();

	mmcsd_select();
	mmcsd_send_cmd(SD_STATUS, 0);

	for (i = 0; i < 64; i++)
		spi_write(0xFF);

	mmcsd_deselect();

	return mmcsd_get_r2(r2);
}

MMCSD_err mmcsd_send_status(uint8_t r2[]) {
	mmcsd_send_cmd(SEND_STATUS, 0);

	return mmcsd_get_r2(r2);
}

MMCSD_err mmcsd_set_blocklen(uint32_t blocklen) {
	mmcsd_send_cmd(SET_BLOCKLEN, blocklen);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_read_single_block(uint32_t address) {
	mmcsd_send_cmd(READ_SINGLE_BLOCK, address);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_write_single_block(uint32_t address) {
	mmcsd_send_cmd(WRITE_BLOCK, address);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_sd_send_op_cond(void) {
	mmcsd_send_cmd(SD_SEND_OP_COND, 0);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_app_cmd(void) {
	mmcsd_send_cmd(APP_CMD, 0);

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_read_ocr(int r3[]) {
	mmcsd_send_cmd(READ_OCR, 0);

	return mmcsd_get_r3(r3);
}

MMCSD_err mmcsd_crc_on_off(int1 crc_enabled) {
	mmcsd_send_cmd(CRC_ON_OFF, crc_enabled);

	g_CRC_enabled = crc_enabled;

	return mmcsd_get_r1();
}

MMCSD_err mmcsd_send_cmd(uint8_t cmd, uint32_t arg) {
	uint8_t packet[6]; // the entire command, argument, and crc in one variable

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
	uint8_t response = 0, // place to hold the response coming back from the SPI line
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

MMCSD_err mmcsd_get_r2(uint8_t r2[]) {
	r2[1] = mmcsd_get_r1();

	r2[0] = spi_read(0xFF);

	return 0;
}

MMCSD_err mmcsd_get_r3(uint8_t r3[]) {
	return mmcsd_get_r7(r3);
}

MMCSD_err mmcsd_get_r7(uint8_t r7[]) {
	uint8_t i;   // counter for loop

	// the top byte of r7 is r1
	r7[4] = mmcsd_get_r1();

	// fill in the other 4 bytes
	for (i = 0; i < 4; i++)
		r7[3 - i] = spi_read(0xFF);

	return r7[4];
}

MMCSD_err mmcsd_wait_for_token(uint8_t token) {
	MMCSD_err r1;

	// get a token
	r1 = mmcsd_get_r1();

	// check to see if the token we recieved was the one that we were looking for
	if (r1 == token)
		return MMCSD_GOODEC;

	// if that wasn't right, return the error
	return r1;
}

unsigned int8 mmcsd_crc7(char *data,uint8_t length)
{
	uint8_t i, ibit, c, crc;

	crc = 0x00;                             // Set initial value

	for (i = 0; i < length; i++, data++)
	{
		c = *data;

		for (ibit = 0; ibit < 8; ibit++)
		{
			crc = crc << 1;
			if ((c ^ crc) & 0x80) crc = crc ^ 0x09; // ^ is XOR
			c = c << 1;
		}

		crc = crc & 0x7F;
	}

	shift_left(&crc, 1, 1); // MMC card stores the result in the top 7 bits so shift them left 1
							// Should shift in a 1 not a 0 as one of the cards I have won't work otherwise
	return crc;
}

uint16_t mmcsd_crc16(char *data, uint8_t length) {
	uint8_t i, ibit, c;

	uint16_t crc;

	crc = 0x0000;                           // Set initial value

	for (i = 0; i < length; i++, data++) {
		c = *data;

		for (ibit = 0; ibit < 8; ibit++) {
			crc = crc << 1;
			if ((c ^ crc) & 0x8000)
				crc = crc ^ 0x1021;                  // ^ is XOR
			c = c << 1;
		}

		crc = crc & 0x7FFF;
	}

	shift_left(&crc, 2, 1); // MMC card stores the result in the top 7 bits so shift them left 1
							// Should shift in a 1 not a 0 as one of the cards I have won't work otherwise
	return crc;
}

void mmcsd_select() {
	output_low(MMCSD_PIN_SELECT);
}

void mmcsd_deselect() {
	spi_write(0xFF);
	output_high(MMCSD_PIN_SELECT);
}

MMCSD_err mmcsd_load_buffer(void) {
	g_MMCSDBufferChanged = FALSE;
	return (mmcsd_read_block(g_mmcsdBufferAddress,
	MMCSD_MAX_BLOCK_SIZE, g_mmcsd_buffer));
}

MMCSD_err mmcsd_flush_buffer(void) {
	if (g_MMCSDBufferChanged) {
		g_MMCSDBufferChanged = FALSE;
		return (mmcsd_write_block(g_mmcsdBufferAddress,
		MMCSD_MAX_BLOCK_SIZE, g_mmcsd_buffer));
	}
	return (0);  //ok
}

MMCSD_err mmcsd_move_buffer(uint32_t new_addr) {
	MMCSD_err ec = MMCSD_GOODEC;
	uint32_t
	//cur_block,
	new_block;

	// make sure we're still on the same block
	//cur_block = g_mmcsdBufferAddress - (g_mmcsdBufferAddress % MMCSD_MAX_BLOCK_SIZE);
	new_block = new_addr - (new_addr % MMCSD_MAX_BLOCK_SIZE);

	//if(cur_block != new_block)
	if (g_mmcsdBufferAddress != new_block) {
		// dump the old buffer
		if (g_MMCSDBufferChanged) {
			ec = mmcsd_flush_buffer();
			if (ec != MMCSD_GOODEC)
				return ec;
			g_MMCSDBufferChanged = FALSE;
		}

		// figure out the best place for a block
		g_mmcsdBufferAddress = new_block;

		// load up a new buffer
		ec = mmcsd_load_buffer();
	}

	return ec;
}

MMCSD_err mmcsd_read_byte(uint32_t addr, char* data) {
	MMCSD_err ec;

	ec = mmcsd_move_buffer(addr);
	if (ec != MMCSD_GOODEC) {
		return ec;
	}

	*data = g_mmcsd_buffer[addr % MMCSD_MAX_BLOCK_SIZE];

	return MMCSD_GOODEC;
}

MMCSD_err mmcsd_write_byte(uint32_t addr, char data) {
	MMCSD_err ec;
	ec = mmcsd_move_buffer(addr);
	if (ec != MMCSD_GOODEC)
		return ec;

	g_mmcsd_buffer[addr % MMCSD_MAX_BLOCK_SIZE] = data;

	g_MMCSDBufferChanged = TRUE;

	return MMCSD_GOODEC;
}

#endif
