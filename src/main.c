#include <18F25K22.h>
#zero_ram
#fuses HSH, NOPLLEN
#use delay(clock=25M)
#use rs232(baud=9600, UART1)
#use spi(master, mode=0, baud=400000, spi1, force_hw, stream=mmcsd_spi)

#define MMCSD_PIN_SELECT PIN_A5

#include "mmcsd.c"
#include <input.c>

int main(void) {
	int r, data[MMCSD_MAX_BLOCK_SIZE];
	long long i;
	short trig, trig1;

	printf("\r\n\nex_mmcsd.c\r\n\n");

	r = mmcsd_init();
	if (r) {
		printf("Could not init the MMC/SD!!!! %u", r);
		while (TRUE)
			;
	} else
		printf("\n\rCartao ok!!");

	while (TRUE) {
		if (!(input(PIN_B0))) {
			delay_ms(100);
			if (!(input(PIN_B0)) && trig) {
				trig = FALSE;
				printf("\n\rTeste Leirura");
				r = mmcsd_read_block(0, MMCSD_MAX_BLOCK_SIZE, data);
				printf(" %u", r);
				for (i = 0; i < MMCSD_MAX_BLOCK_SIZE; ++i) {
					if (!(i % 6))
						printf("\n\r");
					printf(" %03lu-%03u", i, data[i]);

				}
			}
		} else if (!trig)
			trig = TRUE;
		if (!(input(PIN_B1))) {
			delay_ms(100);
			if (!(input(PIN_B1)) && trig1) {
				trig1 = FALSE;
				printf("\n\rTeste Escrita\n\r");
				for (i = 0; i < MMCSD_MAX_BLOCK_SIZE; ++i) {
					data[i] = make8(i, 0);
				}
				r = mmcsd_write_block(0, MMCSD_MAX_BLOCK_SIZE, data);
				printf("\n\r%u", r);
			}
		} else if (!trig1)
			trig1 = TRUE;
	}
	return 0;
}
