/*
 * main.c
 *
 *  Created on: 10 de nov de 2015
 *      Author: cuki
 */

#include <18F25K22.h>

#fuses HSH,NOPLLEN
#use delay(clock=25MHz)
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

	BYTE value, cmd, r;
	int32 address;

	printf("\r\n\nex_mmcsd.c\r\n\n");

	do {
		r = mmcsd_init();
		if (r) {
			printf("\r\nCould not init the MMC/SD!!!!%d", r);
			delay_ms(1000);
		}
	} while (r);

	mmcsd_print_cid();

	while (TRUE) {
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
	}
	return 0;
}
