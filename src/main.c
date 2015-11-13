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

#include "sdcard.c"

int main(void) {

	printf("\r\n\nex_mmcsd.c\r\n\n");

	if (sd_init()) {
		printf("Could not init the MMC/SD!!!!");
	} else
		printf("Ok\n\r");

	while (TRUE)
		;
	return 0;
}
