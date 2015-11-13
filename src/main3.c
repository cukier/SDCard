/*
 * main.c
 *
 *  Created on: 10 de nov de 2015
 *      Author: cuki
 */

#include <18F25K22.h>

#fuses HSM,NOPLLEN
#use delay(clock=25MHz)
#use rs232(baud=9600, UART1)
//#use spi(MASTER, SPI1, BITS=8, MSB_FIRST, MODE=0, baud=400000, stream=mmcsd_spi)

int main(void) {

	setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_CLK_DIV_4);
//	delay_ms(100);

	while (TRUE) {
		spi_write(0x55);
		delay_us(10);
	}

	return 0;
}
