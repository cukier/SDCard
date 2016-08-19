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

#define BUFFER_SIZE		512
#define PATTERN_SIZE	512
#define ADDRESS			0x00000000

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
		if (!(cont % 24) && cont != 0)
			printf("\n\r ");
		if (!(cont % 16) && cont != 0)
			printf(" ");
		else if (!(cont % 8) && cont != 0)
			printf(" ");
		printf("%2x ", arr[cont]);
	}
	printf("\n\r");
}

int main(void) {

	int r;
	int data[BUFFER_SIZE] = { 0 }, teste[PATTERN_SIZE] = { 0 };
	long cont;

	delay_ms(500);
	printf("\n\rSDCard\n\r");
	delay_ms(500);

	r = 0;
	r = mmcsd_init_card();

	if (r) {
		printf("Cartao inicializdo com sucesso!!\n\r");
	} else {
		printf("Cartão não presente ou sem resposta do cartao!\n\r");
	}

	gen_pattern(teste, PATTERN_SIZE, rand());
	r = 0;
	r = mmcsd_write_card(ADDRESS, teste, PATTERN_SIZE);

	if (r) {
		printf("Escrito\n\r  ");
		print_arr(teste, PATTERN_SIZE);
	} else
		printf("Erro ao tentar escrever\n\r");

	r = 0;
	r = mmcsd_read_card(ADDRESS, data, BUFFER_SIZE);

	if (r) {
		printf("Lido\n\r  ");
		print_arr(data, BUFFER_SIZE);
	} else
		printf("Erro ao tentar ler\n\r");

	while (TRUE)
		;

	return 0;
}
