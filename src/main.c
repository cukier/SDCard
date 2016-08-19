#include <18F46K22.h>
#include <STDLIB.H>

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
#use spi(master, mode=0, baud=40000000, spi1)
#endif
#ifdef USE_SERIAL
#use rs232(uart1, baud=19200)
#endif

#include "sdcard.c"

#define BUFFER_SIZE		64
#define PATTERN_SIZE	32

int gen_pattern(int *ptr, long size, long rnd) {
	long cont;

	for (cont = 0; cont < rnd; ++cont)
		rand();

	for (cont = 0; cont < size; ++cont) {
		ptr[cont] = rand();
	}

	return 0;
}

int main(void) {

	long cont, tries;
	int r7[5] = { 0 }, data[BUFFER_SIZE] = { 0 }, teste[PATTERN_SIZE] = { 0 },
			r;

	spi_init(TRUE);
	printf("\n\rSDCard\n\r");
	delay_ms(15);
	r = mmcsd_init(r7);
	spi_init(FALSE);

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
		spi_init(TRUE);
		r = mmcsd_read_block(0, data, BUFFER_SIZE);
		spi_init(FALSE);

		if (r) {
			tries--;
			delay_ms(10);
		}
	} while (tries & r);

	if (tries) {
		if (!r) {
			printf("Lido\n\r  ");
			for (cont = 0; cont < BUFFER_SIZE; ++cont) {
				if (!(cont % 8) && cont != 0)
					printf("\n\r  ");
				printf("%2x ", data[cont]);
			}
			printf("\n\r");
		}
	} else
		printf("Erro ao tentar ler\n\r");

	printf("1: 0x%X 0x%X\n\r\n\r", tries, r);

	gen_pattern(teste, PATTERN_SIZE, rand());
	tries = 0xFF;
	do {
		r = 0xFF;
		spi_init(TRUE);
		r = mmcsd_write_block(0, PATTERN_SIZE, teste);
		spi_init(FALSE);

		if (r) {
			tries--;
			delay_ms(10);
		}
	} while (r != 0 && tries != 0);

	if (tries) {
		if (!r) {
			printf("Escrito\n\r  ");
			for (cont = 0; cont < PATTERN_SIZE; ++cont) {
				if (!(cont % 8) && cont != 0)
					printf("\n\r  ");
				printf("%02x ", teste[cont]);
			}
			printf("\n\r");
		}
	} else
		printf("Erro ao tentar escrever\n\r");

	printf("2: 0x%X 0x%X\n\r\n\r", tries, r);

	while (TRUE)
		;

	return 0;
}
