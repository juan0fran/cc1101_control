#ifndef __CC1101_CONFIG_H__
#define __CC1101_CONFIG_H__

#include "cc1101_utils.h"

int set_radio_parameters(	float freq_hz, float freq_if, 
							radio_modulation_t mod, rate_t data_rate, float mod_index,
							uint8_t packet_length, uint8_t fec, uint8_t white, 		
							preamble_t preamble, sync_word_t sync_word,
							radio_parms_t * radio_parms);

int init_radio_config(spi_parms_t * spi_parms, radio_parms_t * radio_parms);


#endif
