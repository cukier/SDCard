/*
 * sdcard.h
 *
 *  Created on: 27 de abr de 2016
 *      Author: cuki
 */

#ifndef SDCARD_H_
#define SDCARD_H_

#define GO_IDLE_STATE 			0
#define SEND_OP_COND 			1
#define SEND_IF_COND 			8
#define SEND_CSD 				9
#define SEND_CID 				10
#define SD_STATUS 				13
#define SEND_STATUS 			13
#define SET_BLOCKLEN 			16
#define READ_SINGLE_BLOCK 		17
#define WRITE_BLOCK 			24
#define SD_SEND_OP_COND 		41
#define APP_CMD 				55
#define READ_OCR 				58
#define CRC_ON_OFF 				59

#define IDLE_TOKEN				0x01
#define DATA_START_TOKEN		0xFE
#define DUMMY_BYTE				0xFF

#define MMCSD_MAX_BLOCK_SIZE	512
#define MMCSD_BUFFER_SIZE		MMCSD_MAX_BLOCK_SIZE

#define _SS						PIN_A5
#define MMC_DI					PIN_C5

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

extern void mmcsd_deselect(void);
extern void mmcsd_select(void);
extern int mmcsd_crc7(int *data, int length);
extern int mmcsd_send_cmd(int cmd, long long arg, short g_CRC_enabled);
extern int mmcsd_get_r1(void);
extern int mmcsd_get_r2(int *r2);
extern int mmcsd_get_r7(int *r7);
extern int mmcsd_get_r3(int *r3);
extern int mmcsd_go_idle_state(short crc_check);
extern int mmcsd_send_op_cond(short crc_check);
extern int mmcsd_app_cmd(short crc_check);
extern int mmcsd_send_if_cond(short crc_check);
extern int mmcsd_sd_send_op_cond(short crc_check);
extern int mmcsd_sd_send_cmd(int cmd, long long arg, short crc_check, int *r7);
extern int mmcsd_app_send_op_cond(long long arg, short crc_check);
extern int mmcsd_read_ocr(short crc_check, int *r3);
extern int mmcsd_init(int *r7);
extern int mmcsd_read_single_block(long long address);
extern int mmcsd_set_blocken(long long address, long size);
extern int mmcsd_read_block(long long address, int *data, long size);
extern int mmcsd_write_single_block(long long address);
extern int mmcsd_write_block(long long address, long size, int *data, int *err);
extern int mmcsd_test(void);

#endif /* SDCARD_H_ */
