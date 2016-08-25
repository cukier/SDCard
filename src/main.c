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
#include <STDLIBM.H>

#define SIZE_R			1024
#define ADDRESS_R		0
#define SIZE_W			1050
#define ADDRESS_W		120

int main(void) {

	int r;
	int *data, *teste;
	long cont;

#ifndef USE_SPI
	setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_CLK_DIV_4);
#endif

	delay_ms(500);
	printf("\n\rSDCard2\n\r");
	delay_ms(500);

	teste = NULL;
	teste = (int *) malloc((size_t) (SIZE_W * sizeof(int)));

	if (teste == NULL) {
		printf("NO MEM\n\r");
		return 1;
	}

	gen_pattern(teste, SIZE_W, rand());
	r = 0;
	r = mmcsd_write(ADDRESS_W, teste, SIZE_W);

	if (r) {
		printf("Escrito\n\r ");
#ifdef USE_SPI
		print_arr(ADDRESS_W, teste, SIZE_W);
#endif
	} else
		printf("Erro ao tentar escrever\n\r");

	free(teste);

	data = NULL;
	data = (int *) malloc((size_t) (SIZE_R * sizeof(int)));

	if (data == NULL) {
		printf("NO MEM\n\r");
		return 1;
	}

	r = 0;
	r = mmcsd_read(ADDRESS_R, data, SIZE_R);

	if (r) {
		printf("Lido\n\r ");
#ifdef USE_SPI
		print_arr(ADDRESS_R, data, SIZE_R);
#endif
	} else
		printf("Erro ao tentar ler\n\r");

	free(data);

	while (TRUE)
		;

	return 0;
}
