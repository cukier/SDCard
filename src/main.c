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

#define BUFFER_SIZE		520
#define PATTERN_SIZE	10
#define ADDRESS_W		128
#define ADDRESS_R		8

int gen_pattern(int *ptr, long size, long rnd) {
	long cont;

	for (cont = 0; cont < rnd; ++cont)
		rand();

	for (cont = 0; cont < size; ++cont) {
		ptr[cont] = rand();
	}

	return 0;
}

void print_arr(int *arr, long size) {
	long cont;

	for (cont = 0; cont < size; ++cont) {
		if (!(cont % 16) && cont != 0)
			printf("\n ");
		else if (!(cont % 8) && cont != 0)
			printf(" ");
		printf("%2x ", arr[cont]);
	}
	printf("\n");
}

int main(void) {

	int r;
	int data[BUFFER_SIZE] = { 0 }, teste[PATTERN_SIZE] = { 0 };
	long cont;

	delay_ms(500);
	printf("\nSDCard2\n");
	delay_ms(500);

	gen_pattern(teste, PATTERN_SIZE, rand());
	r = 0;
	r = mmcsd_write(ADDRESS_W, teste, PATTERN_SIZE);

	if (r) {
		printf("Escrito\n ");
		print_arr(teste, PATTERN_SIZE);
	} else
		printf("Erro ao tentar escrever\n");

	r = 0;
	r = mmcsd_read(ADDRESS_R, data, BUFFER_SIZE);

	if (r) {
		printf("Lido\n ");
		print_arr(data, BUFFER_SIZE);
	} else
		printf("Erro ao tentar ler\n");

	while (TRUE)
		;

	return 0;
}
