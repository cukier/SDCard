/*
 * main.c
 *
 *  Created on: 10 de nov de 2015
 *      Author: cuki
 */

#include <18F252.h>

#fuses HS
#use delay(clock=25MHz)
#use rs232(baud=9600, UART1)

#include "sdcard.c"

int main(void) {
	int r;

	printf("\r\n\nex_mmcsd.c");
	setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_XMIT_L_TO_H | SPI_CLK_DIV_64);

	do {
		r = init_sd();
		if (r) {
			printf("\r\nCould not init the MMC/SD!!!!%d", r);
			delay_ms(1000);
		}
	} while (r);

	printf("\n\rCartao inicializado ok");

	while (TRUE)
		;

	return 0;
}
