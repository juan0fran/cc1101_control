#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <math.h>

#include "cc1101_routine.h"
#include "cc1101_wrapper.h"

#define TX_FIFO_REFILL 58 // With the default FIFO thresholds selected this is the number of bytes to refill the Tx FIFO
#define RX_FIFO_UNLOAD 59 // With the default FIFO thresholds selected this is the number of bytes to unload from the Rx FIFO

static spi_parms_t spi_parms_it;
radio_int_data_t radio_int_data;
static bool init_radio = false;

static uint8_t rx_aux_buffer[CC11xx_FIFO_SIZE];


static float chanbw_limits[] = {
    812000.0, 650000.0, 541000.0, 464000.0, 406000.0, 325000.0, 270000.0, 232000.0,
    203000.0, 162000.0, 135000.0, 116000.0, 102000.0, 81000.0, 68000.0, 58000.0
};

static uint32_t rate_values[] = {
    50, 110, 300, 600, 1200, 2400, 4800, 9600,
    14400, 19200, 28800, 38400, 57600, 76800, 115200,
};


// ------------------------------------------------------------------------------------------------
// Processes packets up to 255 bytes
void gdo0_isr(void)
// ------------------------------------------------------------------------------------------------
{
    uint8_t int_line, rssi_dec, crc_lqi;
    if (init_radio == false){
        return;
    }
    int_line = CC11xx_GDO0(); // Sense interrupt line to determine if it was a raising or falling edge

    if (radio_int_data.mode == RADIOMODE_RX)
    {
        if (int_line)
        {         
            radio_int_data.byte_index = 0;
            radio_int_data.rx_count = radio_int_data.radio_parms->packet_length;
            radio_int_data.bytes_remaining = radio_int_data.rx_count;
            radio_int_data.packet_receive = 1; // reception is in progress
        }
        else
        {
            if (radio_int_data.packet_receive) // packet has been received
            {
                CC_SPIReadBurstReg(radio_int_data.spi_parms, CC11xx_RXFIFO, rx_aux_buffer, radio_int_data.bytes_remaining);
                memcpy((uint8_t *) &(radio_int_data.rx_buf[radio_int_data.byte_index]), rx_aux_buffer, radio_int_data.bytes_remaining);
                radio_int_data.byte_index += radio_int_data.bytes_remaining;
                radio_int_data.bytes_remaining = 0;

                radio_int_data.mode = RADIOMODE_NONE;
                radio_int_data.packet_receive = 0; // reception is done
                radio_int_data.packet_rx_count++;

                radio_turn_rx_isr(radio_int_data.spi_parms);
            }
            else
            {
                /* Some protection here */
            }
        }    
    }    
    else if (radio_int_data.mode == RADIOMODE_TX)
    {
        if (int_line)
        {
            radio_int_data.packet_send = 1; // Assert packet transmission after sync has been sent
        }
        else
        {
            if (radio_int_data.packet_send) // packet has been sent
            {
                radio_int_data.mode = RADIOMODE_NONE;
                radio_int_data.packet_send = 0; // De-assert packet transmission after packet has been sent
                radio_int_data.packet_tx_count++;

                if ((radio_int_data.bytes_remaining))
                {
                    radio_flush_fifos(radio_int_data.spi_parms);            
                }
                radio_turn_rx_isr(radio_int_data.spi_parms);
            }
            else
            {
                /* Some protection here */
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Processes packets that do not fit in Rx or Tx FIFOs and 255 bytes long maximum
// FIFO threshold interrupt handler 
void gdo2_isr(void)
// ------------------------------------------------------------------------------------------------
{
    uint8_t i, int_line, bytes_to_send;
    if (init_radio == false){
        return;
    }
    int_line = CC11xx_GDO2(); // Sense interrupt line to determine if it was a raising or falling edge

    if ((radio_int_data.mode == RADIOMODE_RX) && (int_line)) // Filling of Rx FIFO - Read next 59 bytes
    {
        if (radio_int_data.packet_receive) // if reception has started
        {
            /* if this shit wants to write but rx_buf will overload, just break */
            CC_SPIReadBurstReg(radio_int_data.spi_parms, CC11xx_RXFIFO, rx_aux_buffer, RX_FIFO_UNLOAD);
            /* Check for status byte in each */
            memcpy((uint8_t *) &(radio_int_data.rx_buf[radio_int_data.byte_index]), rx_aux_buffer, RX_FIFO_UNLOAD);
            radio_int_data.byte_index += RX_FIFO_UNLOAD;
            radio_int_data.bytes_remaining -= RX_FIFO_UNLOAD;            
        }
        else
        {
            /* Some protection here */
        }
    }
    else if ((radio_int_data.mode == RADIOMODE_TX) && (!int_line)) // Depletion of Tx FIFO - Write at most next TX_FIFO_REFILL bytes
    {
        if ((radio_int_data.packet_send) && (radio_int_data.bytes_remaining > 0)) // bytes left to send
        {
            if (radio_int_data.bytes_remaining < TX_FIFO_REFILL)
            {
                bytes_to_send = radio_int_data.bytes_remaining;
            }
            else
            {
                bytes_to_send = TX_FIFO_REFILL;
            }
            CC_SPIWriteBurstReg(radio_int_data.spi_parms, CC11xx_TXFIFO, (uint8_t *) &(radio_int_data.tx_buf[radio_int_data.byte_index]), bytes_to_send);
            /* Check for status byte in each */

            radio_int_data.byte_index += bytes_to_send;
            radio_int_data.bytes_remaining -= bytes_to_send;
        }
        else
        {
            /* Some protection here */
        }
    }
}


int set_radio_parameters(   float freq_hz, float freq_if, 
                            radio_modulation_t mod, rate_t data_rate, float mod_index,
                            uint8_t packet_length, uint8_t fec, uint8_t white,      
                            preamble_t preamble, sync_word_t sync_word,
                            radio_parms_t * radio_parms)
{
    if (freq_hz < 430e6 || freq_hz > 440e6){
        return -1;
    }
    radio_parms->freq_hz        = freq_hz;
    radio_parms->f_if           = freq_if;
    radio_parms->modulation     = mod;
    radio_parms->drate          = data_rate;
    radio_parms->mod_index      = mod_index;
    radio_parms->packet_length  = packet_length;

    if (fec != 0 && fec != 1){
        return -1;
    }
    radio_parms->fec            = fec;
    if (white != 0 && white != 1){
        return -1;
    }

    radio_parms->whitening      = white;
    radio_parms->preamble       = preamble;
    radio_parms->sync_ctl       = sync_word;

    /* Set the nominal parameters */
    radio_parms->f_xtal        = 26000000;
    radio_parms->chanspc_m     = 0;                // Do not use channel spacing for the moment defaulting to 0
    radio_parms->chanspc_e     = 0;                // Do not use channel spacing for the moment defaulting to 0
    return 0;
}

int init_radio_config(spi_parms_t * spi_parms, radio_parms_t * radio_parms)
{
    uint8_t  reg_word;
    // Write register settings
    disable_IT();

    CC_PowerupResetCCxxxx(spi_parms);
    
    /* Patable Write here? */

    // IOCFG2 = 0x00: Set in Rx mode (0x02 for Tx mode)
    // o 0x00: Asserts when RX FIFO is filled at or above the RX FIFO threshold. 
    //         De-asserts when RX FIFO is drained below the same threshold.
    // o 0x02: Asserts when the TX FIFO is filled at or above the TX FIFO threshold.
    //         De-asserts when the TX FIFO is below the same threshold.
    // GDO2 changes depending on the mode
    CC_SPIWriteReg(spi_parms, CC11xx_IOCFG2,   0x00); // GDO2 output pin config.

    // IOCFG0 = 0x06: Asserts when sync word has been sent / received, and de-asserts at the
    // end of the packet. In RX, the pin will de-assert when the optional address
    // check fails or the RX FIFO overflows. In TX the pin will de-assert if the TX
    // FIFO underflows:    
    // GDO0 never changes 
    CC_SPIWriteReg(spi_parms, CC11xx_IOCFG0,   0x06); // GDO0 output pin config.

    // FIFO_THR = 14: 
    // o 5 bytes in TX FIFO (55 available spaces)
    // o 60 bytes in the RX FIFO
    CC_SPIWriteReg(spi_parms, CC11xx_FIFOTHR,  0x0E); // FIFO threshold.

    // PKTLEN: packet length up to 255 bytes. 
    CC_SPIWriteReg(spi_parms, CC11xx_PKTLEN, radio_parms->packet_length); // Packet length.
    
    // PKTCTRL0: Packet automation control #0
    // . bit  7:   unused
    // . bit  6:   0  -> whitening off
    // . bits 5:4: 00 -> normal mode use FIFOs for Rx and Tx
    // . bit  3:   unused
    // . bit  2:   1  -> CRC enabled
    // . bits 1:0: xx -> Packet length mode. Set to FIXED at 255.
    // CRC enabled by default
    reg_word = (radio_parms->whitening<<6) + 0x04;
    CC_SPIWriteReg(spi_parms, CC11xx_PKTCTRL0, reg_word); // Packet automation control.

    // PKTCTRL1: Packet automation control #1
    // . bits 7:5: 000 -> Preamble quality estimator threshold
    // . bit  4:   unused
    // . bit  3:   0   -> Automatic flush of Rx FIFO disabled (too many side constraints see doc)
    // . bit  2:   1   -> Append two status bytes to the payload (RSSI and LQI + CRC OK)
    // . bits 1:0: 00  -> No address check of received packets
    CC_SPIWriteReg(spi_parms, CC11xx_PKTCTRL1, 0x00); // Packet automation control.

    CC_SPIWriteReg(spi_parms, CC11xx_ADDR,     0x00); // Device address for packet filtration (unused, see just above).
    CC_SPIWriteReg(spi_parms, CC11xx_CHANNR,   0x00); // Channel number (unused, use direct frequency programming).

    // FSCTRL0: Frequency offset added to the base frequency before being used by the
    // frequency synthesizer. (2s-complement). Multiplied by Fxtal/2^14
    CC_SPIWriteReg(spi_parms, CC11xx_FSCTRL0,  0x00); // Freq synthesizer control.

    // FSCTRL1: The desired IF frequency to employ in RX. Subtracted from FS base frequency
    // in RX and controls the digital complex mixer in the demodulator. Multiplied by Fxtal/2^10
    // Here 0.3046875 MHz (lowest point below 310 kHz)
    radio_parms->if_word = get_if_word(radio_parms->f_xtal, radio_parms->f_if);
    CC_SPIWriteReg(spi_parms, CC11xx_FSCTRL1, (radio_parms->if_word & 0x1F)); // Freq synthesizer control.

    // FREQ2..0: Base frequency for the frequency sythesizer
    // Fo = (Fxosc / 2^16) * FREQ[23..0]
    // FREQ2 is FREQ[23..16]
    // FREQ1 is FREQ[15..8]
    // FREQ0 is FREQ[7..0]
    // Fxtal = 26 MHz and FREQ = 0x10A762 => Fo = 432.99981689453125 MHz
    radio_parms->freq_word = get_freq_word(radio_parms->f_xtal, radio_parms->freq_hz);
    CC_SPIWriteReg(spi_parms, CC11xx_FREQ2,    ((radio_parms->freq_word>>16) & 0xFF)); // Freq control word, high byte
    CC_SPIWriteReg(spi_parms, CC11xx_FREQ1,    ((radio_parms->freq_word>>8)  & 0xFF)); // Freq control word, mid byte.
    CC_SPIWriteReg(spi_parms, CC11xx_FREQ0,    (radio_parms->freq_word & 0xFF));       // Freq control word, low byte.

    get_rate_words(radio_parms->drate, radio_parms->mod_index, radio_parms);
    // MODCFG4 Modem configuration - bandwidth and data rate exponent
    // High nibble: Sets the decimation ratio for the delta-sigma ADC input stream hence the channel bandwidth
    // . bits 7:6: 0  -> CHANBW_E: exponent parameter (see next)
    // . bits 5:4: 2  -> CHANBW_M: mantissa parameter as per:
    //      BW = Fxosc / 8(4+CHANBW_M).2^CHANBW_E => Here: BW = 26/48 MHz = 541.67 kHz
    //      Factory defaults: M=0, E=1 => BW = 26/128 ~ 203 kHz
    // Low nibble:
    // . bits 3:0: 13 -> DRATE_E: data rate base 2 exponent => here 13 (multiply by 8192)
    reg_word = (radio_parms->chanbw_e<<6) + (radio_parms->chanbw_m<<4) + radio_parms->drate_e;
    CC_SPIWriteReg(spi_parms, CC11xx_MDMCFG4,  reg_word); // Modem configuration.

    // MODCFG3 Modem configuration: DRATE_M data rate mantissa as per formula:
    //    Rate = (256 + DRATE_M).2^DRATE_E.Fxosc / 2^28 
    // Here DRATE_M = 59, DRATE_E = 13 => Rate = 250 kBaud
    CC_SPIWriteReg(spi_parms, CC11xx_MDMCFG3,  radio_parms->drate_m); // Modem configuration.

    // MODCFG2 Modem configuration: DC block, modulation, Manchester, sync word
    // o bit 7:    0   -> Enable DC blocking (1: disable)
    // o bits 6:4: xxx -> (provided)
    // o bit 3:    0   -> Manchester disabled (1: enable)
    // o bits 2:0: 011 -> Sync word qualifier is 30/32 (static init in radio interface)
    reg_word = (get_mod_word(radio_parms->modulation)<<4) + radio_parms->sync_ctl;
    CC_SPIWriteReg(spi_parms, CC11xx_MDMCFG2,  reg_word); // Modem configuration.

    // MODCFG1 Modem configuration: FEC, Preamble, exponent for channel spacing
    // o bit 7:    0   -> FEC disabled (1: enable)
    // o bits 6:4: 2   -> number of preamble bytes (0:2, 1:3, 2:4, 3:6, 4:8, 5:12, 6:16, 7:24)
    // o bits 3:2: unused
    // o bits 1:0: CHANSPC_E: exponent of channel spacing (here: 2)
    reg_word = (radio_parms->fec<<7) + (((int) radio_parms->preamble)<<4) + (radio_parms->chanspc_e);
    CC_SPIWriteReg(spi_parms, CC11xx_MDMCFG1,  reg_word); // Modem configuration.

    // MODCFG0 Modem configuration: CHANSPC_M: mantissa of channel spacing following this formula:
    //    Df = (Fxosc / 2^18) * (256 + CHANSPC_M) * 2^CHANSPC_E
    //    Here: (26 /  ) * 2016 = 0.199951171875 MHz (200 kHz)
    CC_SPIWriteReg(spi_parms, CC11xx_MDMCFG0,  radio_parms->chanspc_m); // Modem configuration.

    // DEVIATN: Modem deviation
    // o bit 7:    0   -> not used
    // o bits 6:4: 0   -> DEVIATION_E: deviation exponent
    // o bit 3:    0   -> not used
    // o bits 2:0: 0   -> DEVIATION_M: deviation mantissa
    //
    //   Modulation  Formula
    //
    //   2-FSK    |  
    //   4-FSK    :  Df = (Fxosc / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E : Here: 1.5869140625 kHz
    //   GFSK     |
    //
    //   MSK      :  Tx: not well documented, Rx: no effect
    //
    //   OOK      : No effect
    //    
    reg_word = (radio_parms->deviat_e<<4) + (radio_parms->deviat_m);
    CC_SPIWriteReg(spi_parms, CC11xx_DEVIATN,  reg_word); // Modem dev (when FSK mod en)

    // MCSM2: Main Radio State Machine. See documentation.
    CC_SPIWriteReg(spi_parms, CC11xx_MCSM2 ,   0x00); //MainRadio Cntrl State Machine

    // MCSM1: Main Radio State Machine. 
    // o bits 7:6: not used
    // o bits 5:4: CCA_MODE: Clear Channel Indicator 
    //   0 (00): Always clear
    //   1 (01): Clear if RSSI below threshold
    //   2 (10): Always claar unless receiving a packet
    //   3 (11): Claar if RSSI below threshold unless receiving a packet
    // o bits 3:2: RXOFF_MODE: Select to what state it should go when a packet has been received
    //   0 (00): IDLE <== 
    //   1 (01): FSTXON
    //   2 (10): TX
    //   3 (11): RX (stay)
    // o bits 1:0: TXOFF_MODE: Select what should happen when a packet has been sent
    //   0 (00): IDLE <==
    //   1 (01): FSTXON
    //   2 (10): TX (stay)
    //   3 (11): RX 
    CC_SPIWriteReg(spi_parms, CC11xx_MCSM1 ,   0x30); //MainRadio Cntrl State Machine

    // MCSM0: Main Radio State Machine.
    // o bits 7:6: not used
    // o bits 5:4: FS_AUTOCAL: When to perform automatic calibration
    //   0 (00): Never i.e. manually via strobe command
    //   1 (01): When going from IDLE to RX or TX (or FSTXON)
    //   2 (10): When going from RX or TX back to IDLE automatically
    //   3 (11): Every 4th time when going from RX or TX to IDLE automatically
    // o bits 3:2: PO_TIMEOUT: 
    //   Value : Exp: Timeout after XOSC start
    //   0 (00):   1: Approx. 2.3 – 2.4 μs
    //   1 (01):  16: Approx. 37 – 39 μs
    //   2 (10):  64: Approx. 149 – 155 μs
    //   3 (11): 256: Approx. 597 – 620 μs
    // o bit 1: PIN_CTRL_EN:   Enables the pin radio control option
    // o bit 0: XOSC_FORCE_ON: Force the XOSC to stay on in the SLEEP state.
    CC_SPIWriteReg(spi_parms, CC11xx_MCSM0 ,   0x18); //MainRadio Cntrl State Machine

    // FOCCFG: Frequency Offset Compensation Configuration.
    // o bits 7:6: not used
    // o bit 5:    If set, the demodulator freezes the frequency offset compensation and clock
    //             recovery feedback loops until the CS signal goes high.
    // o bits 4:3: The frequency compensation loop gain to be used before a sync word is detected.
    //   0 (00): K
    //   1 (01): 2K
    //   2 (10): 3K
    //   3 (11): 4K
    // o bit 2: FOC_POST_K: The frequency compensation loop gain to be used after a sync word is detected.
    //   0: Same as FOC_PRE_K
    //   1: K/2
    // o bits 1:0: FOC_LIMIT: The saturation point for the frequency offset compensation algorithm:
    //   0 (00): ±0 (no frequency offset compensation)
    //   1 (01): ±BW CHAN /8
    //   2 (10): ±BW CHAN /4
    //   3 (11): ±BW CHAN /2
    CC_SPIWriteReg(spi_parms, CC11xx_FOCCFG,   0x1D); // Freq Offset Compens. Config

    // BSCFG:Bit Synchronization Configuration
    // o bits 7:6: BS_PRE_KI: Clock recovery loop integral gain before sync word
    //   0 (00): Ki
    //   1 (01): 2Ki
    //   2 (10): 3Ki
    //   3 (11): 4Ki
    // o bits 5:4: BS_PRE_KP: Clock recovery loop proportional gain before sync word
    //   0 (00): Kp
    //   1 (01): 2Kp
    //   2 (10): 3Kp
    //   3 (11): 4Kp
    // o bit 3: BS_POST_KI: Clock recovery loop integral gain after sync word
    //   0: Same as BS_PRE_KI
    //   1: Ki/2
    // o bit 2: BS_POST_KP: Clock recovery loop proportional gain after sync word
    //   0: Same as BS_PRE_KP
    //   1: Kp
    // o bits 1:0: BS_LIMIT: Data rate offset saturation (max data rate difference)
    //   0 (00): ±0 (No data rate offset compensation performed)
    //   1 (01): ±3.125 % data rate offset
    //   2 (10): ±6.25 % data rate offset
    //   3 (11): ±12.5 % data rate offset
    CC_SPIWriteReg(spi_parms, CC11xx_BSCFG,    0x1C); //  Bit synchronization config.

    // AGCCTRL2: AGC Control
    // o bits 7:6: MAX_DVGA_GAIN. Allowable DVGA settings
    //   0 (00): All gain settings can be used
    //   1 (01): The highest gain setting can not be used
    //   2 (10): The 2 highest gain settings can not be used
    //   3 (11): The 3 highest gain settings can not be used
    // o bits 5:3: MAX_LNA_GAIN. Maximum allowable LNA + LNA 2 gain relative to the maximum possible gain.
    //   0 (000): Maximum possible LNA + LNA 2 gain
    //   1 (001): Approx. 2.6 dB below maximum possible gain
    //   2 (010): Approx. 6.1 dB below maximum possible gain
    //   3 (011): Approx. 7.4 dB below maximum possible gain
    //   4 (100): Approx. 9.2 dB below maximum possible gain
    //   5 (101): Approx. 11.5 dB below maximum possible gain
    //   6 (110): Approx. 14.6 dB below maximum possible gain
    //   7 (111): Approx. 17.1 dB below maximum possible gain
    // o bits 2:0: MAGN_TARGET: target value for the averaged amplitude from the digital channel filter (1 LSB = 0 dB).
    //   0 (000): 24 dB
    //   1 (001): 27 dB
    //   2 (010): 30 dB
    //   3 (011): 33 dB
    //   4 (100): 36 dB
    //   5 (101): 38 dB
    //   6 (110): 40 dB
    //   7 (111): 42 dB
    CC_SPIWriteReg(spi_parms, CC11xx_AGCCTRL2, 0xC7); // AGC control.

    // AGCCTRL1: AGC Control
    // o bit 7: not used
    // o bit 6: AGC_LNA_PRIORITY: Selects between two different strategies for LNA and LNA 2 gain
    //   0: the LNA 2 gain is decreased to minimum before decreasing LNA gain
    //   1: the LNA gain is decreased first.
    // o bits 5:4: CARRIER_SENSE_REL_THR: Sets the relative change threshold for asserting carrier sense
    //   0 (00): Relative carrier sense threshold disabled
    //   1 (01): 6 dB increase in RSSI value
    //   2 (10): 10 dB increase in RSSI value
    //   3 (11): 14 dB increase in RSSI value
    // o bits 3:0: CARRIER_SENSE_ABS_THR: Sets the absolute RSSI threshold for asserting carrier sense. 
    //   The 2-complement signed threshold is programmed in steps of 1 dB and is relative to the MAGN_TARGET setting.
    //   0 is at MAGN_TARGET setting.
    CC_SPIWriteReg(spi_parms, CC11xx_AGCCTRL1, 0x00); // AGC control.

    // AGCCTRL0: AGC Control
    // o bits 7:6: HYST_LEVEL: Sets the level of hysteresis on the magnitude deviation
    //   0 (00): No hysteresis, small symmetric dead zone, high gain
    //   1 (01): Low hysteresis, small asymmetric dead zone, medium gain
    //   2 (10): Medium hysteresis, medium asymmetric dead zone, medium gain
    //   3 (11): Large hysteresis, large asymmetric dead zone, low gain
    // o bits 5:4: WAIT_TIME: Sets the number of channel filter samples from a gain adjustment has
    //   been made until the AGC algorithm starts accumulating new samples.
    //   0 (00):  8
    //   1 (01): 16
    //   2 (10): 24
    //   3 (11): 32
    // o bits 3:2: AGC_FREEZE: Control when the AGC gain should be frozen.
    //   0 (00): Normal operation. Always adjust gain when required.
    //   1 (01): The gain setting is frozen when a sync word has been found.
    //   2 (10): Manually freeze the analogue gain setting and continue to adjust the digital gain. 
    //   3 (11): Manually freezes both the analogue and the digital gain setting. Used for manually overriding the gain.
    // o bits 0:1: FILTER_LENGTH: 
    //   2-FSK, 4-FSK, MSK: Sets the averaging length for the amplitude from the channel filter.    |  
    //   ASK ,OOK: Sets the OOK/ASK decision boundary for OOK/ASK reception.
    //   Value : #samples: OOK/ASK decixion boundary
    //   0 (00):        8: 4 dB
    //   1 (01):       16: 8 dB
    //   2 (10):       32: 12 dB
    //   3 (11):       64: 16 dB  
    CC_SPIWriteReg(spi_parms, CC11xx_AGCCTRL0, 0xB2); // AGC control.

    // FREND1: Front End RX Configuration
    // o bits 7:6: LNA_CURRENT: Adjusts front-end LNA PTAT current output
    // o bits 5:4: LNA2MIX_CURRENT: Adjusts front-end PTAT outputs
    // o bits 3:2: LODIV_BUF_CURRENT_RX: Adjusts current in RX LO buffer (LO input to mixer)
    // o bits 1:0: MIX_CURRENT: Adjusts current in mixer
    CC_SPIWriteReg(spi_parms, CC11xx_FREND1,   0xB6); // Front end RX configuration.

    // FREND0: Front End TX Configuration
    // o bits 7:6: not used
    // o bits 5:4: LODIV_BUF_CURRENT_TX: Adjusts current TX LO buffer (input to PA). The value to use
    //   in this field is given by the SmartRF Studio software
    // o bit 3: not used
    // o bits 1:0: PA_POWER: Selects PA power setting. This value is an index to the PATABLE, 
    //   which can be programmed with up to 8 different PA settings. In OOK/ASK mode, this selects the PATABLE
    //   index to use when transmitting a ‘1’. PATABLE index zero is used in OOK/ASK when transmitting a ‘0’. 
    //   The PATABLE settings from index ‘0’ to the PA_POWER value are used for ASK TX shaping, 
    //   and for power ramp-up/ramp-down at the start/end of transmission in all TX modulation formats.
    CC_SPIWriteReg(spi_parms, CC11xx_FREND0,   0x10); // Front end RX configuration.

    // FSCAL3: Frequency Synthesizer Calibration
    // o bits 7:6: The value to write in this field before calibration is given by the SmartRF
    //   Studio software.
    // o bits 5:4: CHP_CURR_CAL_EN: Disable charge pump calibration stage when 0.
    // o bits 3:0: FSCAL3: Frequency synthesizer calibration result register.
    CC_SPIWriteReg(spi_parms, CC11xx_FSCAL3,   0xEA); // Frequency synthesizer cal.

    // FSCAL2: Frequency Synthesizer Calibration
    CC_SPIWriteReg(spi_parms, CC11xx_FSCAL2,   0x0A); // Frequency synthesizer cal.
    CC_SPIWriteReg(spi_parms, CC11xx_FSCAL1,   0x00); // Frequency synthesizer cal.
    CC_SPIWriteReg(spi_parms, CC11xx_FSCAL0,   0x11); // Frequency synthesizer cal.
    CC_SPIWriteReg(spi_parms, CC11xx_FSTEST,   0x59); // Frequency synthesizer cal.

    // TEST2: Various test settings. The value to write in this field is given by the SmartRF Studio software.
    CC_SPIWriteReg(spi_parms, CC11xx_TEST2,    0x88); // Various test settings.

    // TEST1: Various test settings. The value to write in this field is given by the SmartRF Studio software.
    CC_SPIWriteReg(spi_parms, CC11xx_TEST1,    0x31); // Various test settings.

    // TEST0: Various test settings. The value to write in this field is given by the SmartRF Studio software.
    CC_SPIWriteReg(spi_parms, CC11xx_TEST0,    0x09); // Various test settings.

    enable_IT();

    return 0;
}

int set_freq(spi_parms_t * spi_parms, radio_parms_t * radio_parms)
{
    // FREQ2..0: Base frequency for the frequency sythesizer
    // Fo = (Fxosc / 2^16) * FREQ[23..0]
    // FREQ2 is FREQ[23..16]
    // FREQ1 is FREQ[15..8]
    // FREQ0 is FREQ[7..0]
    // Fxtal = 26 MHz and FREQ = 0x10A762 => Fo = 432.99981689453125 MHz
    radio_parms->freq_word = get_freq_word(radio_parms->f_xtal, radio_parms->freq_hz);
    CC_SPIWriteReg(spi_parms, CC11xx_FREQ2,    ((radio_parms->freq_word>>16) & 0xFF)); // Freq control word, high byte
    CC_SPIWriteReg(spi_parms, CC11xx_FREQ1,    ((radio_parms->freq_word>>8)  & 0xFF)); // Freq control word, mid byte.
    CC_SPIWriteReg(spi_parms, CC11xx_FREQ0,    (radio_parms->freq_word & 0xFF));       // Freq control word, low byte.
    return 0;
}


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
}

// ------------------------------------------------------------------------------------------------
// Calculate data rate, channel bandwidth and deviation words. Assumes 26 MHz crystal.
//   o DRATE = (Fxosc / 2^28) * (256 + DRATE_M) * 2^DRATE_E
//   o CHANBW = Fxosc / (8(4+CHANBW_M) * 2^CHANBW_E)
//   o DEVIATION = (Fxosc / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E
void get_rate_words(rate_t data_rate, float mod_index, radio_parms_t *radio_parms)
// ------------------------------------------------------------------------------------------------
{
    float drate, deviat, f_xtal;

    drate = (float) rate_values[data_rate];

    deviat = drate * mod_index;
    f_xtal = (float) radio_parms->f_xtal;

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
    radio_flush_fifos(spi_parms);
}

void radio_turn_rx_isr(spi_parms_t *spi_parms)
{
    CC_SPIWriteReg(spi_parms, CC11xx_IOCFG2, 0x00); // GDO2 output pin config RX mode
    radio_int_data.packet_receive = 0;
    radio_int_data.mode = RADIOMODE_RX;   
    CC_SPIStrobe(spi_parms, CC11xx_SRX);
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
    uint8_t  cca, cca_count;

    radio_int_data.mode = RADIOMODE_NONE;
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
		cca = CC11xx_GDO2();
	}while((cca == 0) && (cca_count < 3)); 	/* If CCA == 0 (channel is not clear) remain in the loop */
									 		/* If cca_count is < 3 remain in the loop */
	if (cca_count >= 3 && cca == 0){
		/* If the limit is completed -> go out */
        /* Put this shit into RX */
        radio_turn_idle(spi_parms);
        radio_init_rx(spi_parms);
        radio_turn_rx(spi_parms);
		return;
	}

	/* Here is TX */
	CC_SPIWriteReg(spi_parms, CC11xx_IOCFG2,   0x02); // GDO2 output pin config TX mode
    radio_int_data.mode = RADIOMODE_TX;
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
    /* Timeout? */
    while(radio_int_data.mode == RADIOMODE_TX || radio_int_data.packet_receive)
    {
        MSLEEP(1);
    }
    radio_turn_idle(spi_parms);

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
	radio_int_data.spi_parms = &spi_parms_it;
	radio_int_data.radio_parms = radio_parms;
    init_radio = true;
	/* enable RX! */
    radio_init_rx(radio_int_data.spi_parms, radio_int_data.radio_parms);
	radio_turn_rx(radio_int_data.spi_parms);
}


int  CC_SPIWriteReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t byte)
{
    spi_parms->tx[0] = addr;
    spi_parms->tx[1] = byte;
    spi_parms->len = 2;
    spi_parms->ret = SPI_TRANSFER(spi_parms->tx, spi_parms->rx, spi_parms->len);
    if (spi_parms->ret != 0){
        return 1;
    }
    spi_parms->status = spi_parms->rx[0];
    return 0;
}

/* Correct write burst to be multiple calls to write reg */

int  CC_SPIWriteBurstReg(spi_parms_t *spi_parms, uint8_t addr, const uint8_t *bytes, uint8_t count)
{
    uint8_t i;

    count %= 64;
    spi_parms->tx[0] = addr | CC11xx_WRITE_BURST;   // Send address

    for (i=1; i<count+1; i++)
    {
        spi_parms->tx[i] = bytes[i-1];
    }
    spi_parms->len = count+1;

    spi_parms->ret = SPI_TRANSFER(spi_parms->tx, spi_parms->rx, spi_parms->len);

    if (spi_parms->ret != 0){
        return 1;
    }
    spi_parms->status = spi_parms->rx[0];
    return 0;
}

int  CC_SPIReadReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t *byte)
{
    spi_parms->tx[0] = addr | CC11xx_READ_SINGLE; // Send address
    spi_parms->tx[1] = 0; // Dummy write so we can read data
    spi_parms->len = 2;

    spi_parms->ret = SPI_TRANSFER(spi_parms->tx, spi_parms->rx, spi_parms->len);

    if (spi_parms->ret != 0){
        return 1;
    }
    *byte = spi_parms->rx[1];
    spi_parms->status = spi_parms->rx[0];
    return 0;
}

/* Correct read burst to be multiple calls to read reg */

int  CC_SPIReadBurstReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t **bytes, uint8_t count)
{    
    uint8_t i;

    count %= 64;
    spi_parms->tx[0] = addr | CC11xx_READ_BURST;   // Send address

    for (i=1; i<count+1; i++)
    {
        spi_parms->tx[i] = 0; // Dummy write so we can read data
    }

    spi_parms->len = count+1;
    spi_parms->ret = SPI_TRANSFER(spi_parms->tx, spi_parms->rx, spi_parms->len);

    if (spi_parms->ret != 0)
    {
        return 1;
    }

    *bytes = &spi_parms->rx[1];
    spi_parms->status = spi_parms->rx[0];
    return 0;
}

int  CC_SPIReadStatus(spi_parms_t *spi_parms, uint8_t addr, uint8_t *status)
{
    spi_parms->tx[0] = addr | CC11xx_READ_BURST;   // Send address
    spi_parms->tx[1] = 0; // Dummy write so we can read data
    spi_parms->len = 2;

    spi_parms->ret = SPI_TRANSFER(spi_parms->tx, spi_parms->rx, spi_parms->len);

    if (spi_parms->ret != 0)
    {
        return 1;
    }
    *status = spi_parms->rx[1];
    spi_parms->status = spi_parms->rx[0];
    return 0;
}

int  CC_SPIStrobe(spi_parms_t *spi_parms, uint8_t strobe)
{
    spi_parms->tx[0] = strobe;   // Send strobe
    spi_parms->len = 1;

    spi_parms->ret = SPI_TRANSFER(spi_parms->tx, spi_parms->rx, spi_parms->len);

    if (spi_parms->ret != 0)
    {
        return 1;
    }
    spi_parms->status = spi_parms->rx[0];
    return 0;
}

int  CC_PowerupResetCCxxxx(spi_parms_t *spi_parms)
{
    return CC_SPIStrobe(spi_parms, CC11xx_SRES);
}


void disable_IT(void)
{
    /* Must be changed */

}

void enable_IT(void)
{


}