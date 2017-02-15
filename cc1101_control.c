#include "cc1101_config.h"
#include "cc1101_utils.h"
/* With that, is all included */

int main (void)
{
	radio_parms_t radio_handler;
	spi_parms_t spi_handler;

	set_radio_parameters(433.92e6, 310000, RADIO_MOD_FSK2, RATE_9600, 0.5, 255, 0, 1, PREAMBLE_4, SYNC_30_over_32, &radio_handler);

	init_radio_config(&spi_handler, &radio_handler);

	enable_isr_routine(&spi_handler, &radio_handler);
}

