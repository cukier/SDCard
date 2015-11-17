/*
 * main.c
 *
 *  Created on: 10 de nov de 2015
 *      Author: cuki
 *
 *      http://pwc.theclarkwebsite.com/sdcard.php
 */

#include <18F25K22.h>

#fuses HSM,NOPLLEN
#use delay(clock=4MHz)
#use rs232(baud=9600, UART1)

#define _SS PIN_A5

int assert(void) {
	int cont = 10, r = 0;

	output_high(_SS);
	while (--cont)
		spi_write(0xFF);
	output_low(_SS);
	spi_write(0x40);
	spi_write(0x00);
	spi_write(0x00);
	spi_write(0x00);
	spi_write(0x00);
	spi_write(0x95);
	r = spi_read(0xFF);
	output_high(_SS);

	return r;
}

int send_cmd(int cmd) {

	output_low(_SS);
	spi_write(cmd);
	output_high(_SS);
	return 0;
}

int main(void) {

	output_high(_SS);
	setup_spi(SPI_MASTER | SPI_H_TO_L | SPI_CLK_DIV_4);
	delay_ms(100);

	printf("\n\rAssert %u", assert());

	while (TRUE)
		;

	return 0;
}

