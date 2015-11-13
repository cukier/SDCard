/*
 * main.c
 *
 *  Created on: 10 de nov de 2015
 *      Author: cuki
 */

#include <18F45K22.h>

#fuses HSH,NOPLLEN
#use delay(clock=15MHz)
#use rs232(baud=9600, UART1)

#include <stdlib.h>

//meda library, a compatable media library is required for FAT.
#use fast_io(c)
#define MMCSD_PIN_SCL     PIN_C3 //o
#define MMCSD_PIN_SDI     PIN_C4 //i
#define MMCSD_PIN_SDO     PIN_C5 //o
#define MMCSD_PIN_SELECT  PIN_A5 //o
//#define MMCSD_SPI_HW  SPI1

//#define MMCSD_SPI_XFER
#include <mmcsd.c>

#include <input.c>

int main(void) {

	BYTE value, cmd;
	int32 address;

	printf("\r\n\nex_mmcsd.c\r\n\n");

	if (mmcsd_init()) {
		printf("Could not init the MMC/SD!!!!");
		while (TRUE)
			;
	}

	do {
		do {
			printf("\r\nRead or Write: ");
			cmd = getc();
			cmd = toupper(cmd);
			putc(cmd);
		} while ((cmd != 'R') && (cmd != 'W'));

		printf("\n\rLocation: ");

		address = gethex();
		address = (address << 8) + gethex();

		if (cmd == 'R') {
			mmcsd_read_byte(address, &value);
			printf("\r\nValue: %X\r\n", value);
		}

		if (cmd == 'W') {
			printf("\r\nNew value: ");
			value = gethex();
			printf("\n\r");
			mmcsd_write_byte(address, value);
			mmcsd_flush_buffer();
		}
	} while (TRUE);
	return 0;
}
