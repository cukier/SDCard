#include <18F46K22.h>

#define USE_PLL
//#define USE_WDT
#define USE_SPI
#define USE_SERIAL

#ifdef USE_PLL
#ifdef USE_WDT
#fuses  mclr, primary_on, intrc, pllen, wdt, wdt1024
#else
#fuses  mclr, primary_on, intrc, pllen, nowdt
#endif
#use delay(internal=64MHz)
#else
#ifdef USE_WDT
#fuses  mclr, primary_on, intrc, nopllen, wdt, wdt2048
#else
#fuses  mclr, primary_on, intrc, nopllen, nowdt
#endif
#use delay(internal=16MHz)
#endif
#ifdef USE_SPI
#use spi(master, mode=0, baud=20000, spi1, stream=SPI)
#endif
#ifdef USE_SERIAL
#use rs232(uart1, baud=19200)
#endif

#include "sdcard.c"

#define BUFFER_SIZE	8

void print_array(int *array, long size) {
	long cont;

	for (cont = 0; cont < BUFFER_SIZE; ++cont) {
#ifdef USE_WDT
		restart_wdt();
#endif
		if (!(cont % 8))
			printf("\n\r");
		printf(" %03lu:0x%X", cont, array[cont]);
	}

	return;
}

int main(void) {

	long cont, tries;
	int r7[5] = { 0 }, data[BUFFER_SIZE] = { 0 }, teste[BUFFER_SIZE] = { 'H',
			'e', 'l', 'l', 'o', ' ', 'i', 'o' }, r, err;

	spi_init(SPI, TRUE);
	printf("\n\rSDCard\n\r");
	delay_ms(15);
	r = mmcsd_init(r7);

	if (r != RESP_TIMEOUT && r != 0xFF) {
		printf("%u === \n\r", r);
		for (cont = 0; cont < 5; ++cont)
			printf(" %x", r7[4 - cont]);
		printf("\n\rCartao inicializdo com sucesso!!\n\r");
	} else {
		printf("\n\rCartão não presente ou sem resposta do cartao!\n\r");
	}

	tries = 0xFF;
	do {
		r = 0xFF;
		r = mmcsd_read_block(0, data, BUFFER_SIZE);

		if (r) {
			tries--;
			delay_ms(100);
		}
	} while (tries & r);

	if (tries) {
		if (!r) {
			printf("Lido 0x%X\n\r", tries);
			for (cont = 0; cont < BUFFER_SIZE; ++cont)
				printf("%c ", data[cont]);
			printf("\n\r");
		}
	}

	printf("1: 0x%X 0x%X\n\r", tries, r);

	tries = 0xFF;
	do {
		r = 0xFF;
		r = mmcsd_write_block(0, BUFFER_SIZE, teste, &err);

		if (r) {
			tries--;
			delay_ms(100);
		}
	} while (r != 0 & tries != 0);

	printf("2: 0x%X 0x%X\n\r", tries, r);

	if (tries) {
		if (!r) {
			printf("Escrito 0x%X\n\r", tries);
		}
	} else
		printf("Erro ao tentar escrever\n\r");

	printf("Fim 0x%X\n\r", tries);

	while (TRUE)
		;

	return 0;
}
