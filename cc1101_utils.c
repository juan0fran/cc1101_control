#include "cc1101_utils.h"
#include <math.h>

static radio_int_data_t radio_int_data;
radio_int_data_t * p_radio_int_data = NULL;

static float chanbw_limits[] = {
    812000.0, 650000.0, 541000.0, 464000.0, 406000.0, 325000.0, 270000.0, 232000.0,
    203000.0, 162000.0, 135000.0, 116000.0, 102000.0, 81000.0, 68000.0, 58000.0
};

static uint32_t rate_values[] = {
    50, 110, 300, 600, 1200, 2400, 4800, 9600,
    14400, 19200, 28800, 38400, 57600, 76800, 115200,
};

static uint8_t nb_preamble_bytes[] = {
    2, 3, 4, 6, 8, 12, 16, 24
};

// ------------------------------------------------------------------------------------------------
// Calculate RSSI in dBm from decimal RSSI read out of RSSI status register
float rssi_dbm(uint8_t rssi_dec)
// ------------------------------------------------------------------------------------------------
{
    if (rssi_dec < 128)
    {
        return (rssi_dec / 2.0) - 74.0;
    }
    else
    {
        return ((rssi_dec - 256) / 2.0) - 74.0;
    }
}

// ------------------------------------------------------------------------------------------------
// Calculate frequency word FREQ[23..0]
uint32_t get_freq_word(uint32_t freq_xtal, uint32_t freq_hz)
// ------------------------------------------------------------------------------------------------
{
    uint64_t res; // calculate on 64 bits to save precision
    res = ((uint64_t) freq_hz * (uint64_t) (1<<16)) / ((uint64_t) freq_xtal);
    return (uint32_t) res;
}

// ------------------------------------------------------------------------------------------------
// Calculate frequency word FREQ[23..0]
uint32_t get_if_word(uint32_t freq_xtal, uint32_t if_hz)
// ------------------------------------------------------------------------------------------------
{
    return (if_hz * (1<<10)) / freq_xtal;
}

// ------------------------------------------------------------------------------------------------
// Calculate modulation format word MOD_FORMAT[2..0]
uint8_t get_mod_word(radio_modulation_t modulation_code)
// ------------------------------------------------------------------------------------------------
{
    switch (modulation_code)
    {
        case RADIO_MOD_OOK:
            return 3;
            break;
        case RADIO_MOD_FSK2:
            return 0;
            break;
        case RADIO_MOD_FSK4:
            return 4;
            break;
        case RADIO_MOD_MSK:
            return 7;
            break;
        case RADIO_MOD_GFSK:
            return 1;
            break;
        default:
            return 0;
    }
}

// ------------------------------------------------------------------------------------------------
// Calculate CHANBW words according to CC1101 bandwidth steps
void get_chanbw_words(float bw, radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    uint8_t e_index, m_index;

    for (e_index=0; e_index<4; e_index++)
    {
        for (m_index=0; m_index<4; m_index++)
        {
            if (bw > chanbw_limits[4*e_index + m_index])
            {
                radio_parms->chanbw_e = e_index;
                radio_parms->chanbw_m = m_index;
                return;
            }
        }
    }

    radio_parms->chanbw_e = 3;
    radio_parms->chanbw_m = 3;
}

// ------------------------------------------------------------------------------------------------
// Calculate data rate, channel bandwidth and deviation words. Assumes 26 MHz crystal.
//   o DRATE = (Fxosc / 2^28) * (256 + DRATE_M) * 2^DRATE_E
//   o CHANBW = Fxosc / (8(4+CHANBW_M) * 2^CHANBW_E)
//   o DEVIATION = (Fxosc / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E
void get_rate_words(rate_t data_rate, float mod_index, radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    double drate, deviat, f_xtal;

    drate = (double) rate_values[data_rate];

    deviat = drate * mod_index;
    f_xtal = (double) radio_parms->f_xtal;

    get_chanbw_words(2.0*(deviat + drate), radio_parms); // Apply Carson's rule for bandwidth

    radio_parms->drate_e = (uint8_t) (floor(log2( drate*(1<<20) / f_xtal )));
    radio_parms->drate_m = (uint8_t) (((drate*(1<<28)) / (f_xtal * (1<<radio_parms->drate_e))) - 256);
    radio_parms->drate_e &= 0x0F; // it is 4 bits long

    radio_parms->deviat_e = (uint8_t) (floor(log2( deviat*(1<<14) / f_xtal )));
    radio_parms->deviat_m = (uint8_t) (((deviat*(1<<17)) / (f_xtal * (1<<radio_parms->deviat_e))) - 8);
    radio_parms->deviat_e &= 0x07; // it is 3 bits long
    radio_parms->deviat_m &= 0x07; // it is 3 bits long

    radio_parms->chanspc_e &= 0x03; // it is 2 bits long
}

// ------------------------------------------------------------------------------------------------
// Poll FSM state waiting for given state until timeout (approx ms)
void wait_for_state(spi_parms_t *spi_parms, CC11xx_state_t state, uint32_t timeout)
// ------------------------------------------------------------------------------------------------
{
    uint8_t fsm_state = 0x00;

    while(timeout)
    {
        CC_SPIReadStatus(spi_parms, CC11xx_MARCSTATE, &fsm_state);
        fsm_state &= 0x1F;

        if (fsm_state == (uint8_t) state)
        {
            break;
        }

        MSLEEP(1);
        timeout--;
    }

    if (!timeout)
    {       
        if (fsm_state == CC11xx_STATE_RXFIFO_OVERFLOW)
        {
            CC_SPIStrobe(spi_parms, CC11xx_SFRX); // Flush Rx FIFO
            CC_SPIStrobe(spi_parms, CC11xx_SFTX); // Flush Tx FIFO
        }
    }    
}

// ------------------------------------------------------------------------------------------------
// Inhibit operations by returning to IDLE state
void radio_turn_idle(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    CC_SPIStrobe(spi_parms, CC11xx_SIDLE);
}

// ------------------------------------------------------------------------------------------------
// Allow Rx operations by returning to Rx state
void radio_turn_rx(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    CC_SPIStrobe(spi_parms, CC11xx_SRX);
    wait_for_state(spi_parms, CC11xx_STATE_RX, 10); // Wait max 10ms
}

// ------------------------------------------------------------------------------------------------
// Initialize for Rx mode
void radio_init_rx(spi_parms_t *spi_parms, radio_parms_t * radio_parms)
// ------------------------------------------------------------------------------------------------
{
    radio_int_data.mode = RADIOMODE_RX;
    radio_int_data.packet_receive = 0;    
    radio_set_packet_length(spi_parms, radio_parms->packet_length);
    CC_SPIWriteReg(spi_parms, CC11xx_IOCFG2, 0x00); // GDO2 output pin config RX mode
}

// ------------------------------------------------------------------------------------------------
// Flush Rx and Tx FIFOs
void radio_flush_fifos(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    CC_SPIStrobe(spi_parms, CC11xx_SFRX); // Flush Rx FIFO
    CC_SPIStrobe(spi_parms, CC11xx_SFTX); // Flush Tx FIFO
}

// ------------------------------------------------------------------------------------------------
// Set packet length
int radio_set_packet_length(spi_parms_t *spi_parms, uint8_t pkt_len)
// ------------------------------------------------------------------------------------------------
{
    return CC_SPIWriteReg(spi_parms, CC11xx_PKTLEN, pkt_len); // Packet length.
}

// ------------------------------------------------------------------------------------------------
// Get packet length
uint8_t radio_get_packet_length(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    uint8_t pkt_len;
    CC_SPIReadReg(spi_parms, CC11xx_PKTLEN, &pkt_len); // Packet length.
    return pkt_len;
}

// ------------------------------------------------------------------------------------------------
// Get the actual data rate in Bauds
float radio_get_rate(radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    return ((float) (radio_parms->f_xtal) / (1<<28)) * (256 + radio_parms->drate_m) * (1<<radio_parms->drate_e);
}

// ------------------------------------------------------------------------------------------------
// Transmission of a block
static void radio_send_block(spi_parms_t *spi_parms)
// ------------------------------------------------------------------------------------------------
{
    uint8_t  initial_tx_count; // Number of bytes to send in first batch
    int      i, ret;
    uint8_t  cca, cca_count;
    radio_int_data.mode = RADIOMODE_TX;
    radio_int_data.packet_send = 0;

    radio_set_packet_length(spi_parms, radio_int_data.tx_count);
    /* Set this shit to CCA --> Poll for this? Use ISR? */
    /* Lets start by polling GDO2 pin, if is 1, then Random Back off, look for 1 again and go! */
    /* The radio is always in RX */
    CC_SPIWriteReg(spi_parms, CC11xx_IOCFG2,   0x09); // GDO2 output pin config TX mode
    radio_turn_rx(spi_parms);

    MDELAY(1);
    /* Wait 1 ms to the RX to be set */
	cca_count = 0;
	do{
		MDELAY(rand()%256);
		cca_count++;
		cca = CC11xx_PIN(GDO2_pin);
	}while((cca == 0) && (cca_count < 3)); 	/* If CCA == 0 (channel is not clear) remain in the loop */
									 		/* If cca_count is < 3 remain in the loop */
	if (cca_count >= 3 && cca == 0){
		/* If the limit is completed -> go out */
		return;
	}

	/* Here is TX */
	CC_SPIWriteReg(spi_parms, CC11xx_IOCFG2,   0x02); // GDO2 output pin config TX mode

    // Initial number of bytes to put in FIFO is either the number of bytes to send or the FIFO size whichever is
    // the smallest. Actual size blocks you need to take size minus one byte.
    initial_tx_count = (radio_int_data.tx_count > CC11xx_FIFO_SIZE-1 ? CC11xx_FIFO_SIZE-1 : radio_int_data.tx_count);
    // Initial fill of TX FIFO
    CC_SPIWriteBurstReg(spi_parms, CC11xx_TXFIFO, (uint8_t *) radio_int_data.tx_buf, initial_tx_count);
    radio_int_data.byte_index = initial_tx_count;
    radio_int_data.bytes_remaining = radio_int_data.tx_count - initial_tx_count;
 	CC_SPIStrobe(spi_parms, CC11xx_STX); // Kick-off Tx
}

// ------------------------------------------------------------------------------------------------
// Transmission of a packet
void radio_send_packet(spi_parms_t *spi_parms, radio_parms_t * radio_parms, uint8_t *packet, uint8_t size)
// ------------------------------------------------------------------------------------------------
{
    radio_int_data.tx_count = radio_parms->packet_length; // same block size for all
    memset((uint8_t *) &radio_int_data.tx_buf[0], 0, radio_parms->packet_length);
    memcpy((uint8_t *) &radio_int_data.tx_buf[0], packet, size);
    radio_send_block(spi_parms);
}

void enable_isr_routine(spi_parms_t * spi, radio_parms_t * radio_parms)
{
	radio_int_data.mode = RADIOMODE_NONE;
    radio_int_data.packet_rx_count = 0;
    radio_int_data.packet_tx_count = 0;
	radio_int_data.spi_parms = spi;
	radio_int_data.radio_parms = radio_parms;
	p_radio_int_data = &radio_int_data;
	/* enable RX! */
    radio_init_rx(radio_int_data.spi_parms, radio_int_data.radio_parms);
	radio_turn_rx(radio_int_data.spi_parms);
}







