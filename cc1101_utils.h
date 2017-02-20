#ifndef __CC1101_UTILS_H__
#define __CC1101_UTILS_H__

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cc1101_defines.h"
#include "cc1101_low_level.h"

#define MSLEEP(x) usleep(x*1000)
#define MDELAY(x) MSLEEP(x)

/* Preamble amount */
typedef enum preamble_e {
    PREAMBLE_2,
    PREAMBLE_3,
    PREAMBLE_4,
    PREAMBLE_6,
    PREAMBLE_8,
    PREAMBLE_12,
    PREAMBLE_16,
    PREAMBLE_24,
    NUM_PREAMBLE
} preamble_t;

/* Sync word type */
typedef enum sync_word_e
{
    NO_SYNC = 0,              // No preamble/sync
    SYNC_15_OVER_16,          // 15/16 sync word bits detected
    SYNC_16_OVER_16,          // 16/16 sync word bits detected
    SYNC_30_over_32,          // 30/32 sync word bits detected
    SYNC_CARRIER,             // No preamble/sync, carrier-sense above threshold
    SYNC_15_OVER_16_CARRIER,  // 15/16 + carrier-sense above threshold
    SYNC_16_OVER_16_CARRIER,  // 16/16 + carrier-sense above threshold
    SYNC_30_over_32_CARRIER   // 30/32 + carrier-sense above threshold
} sync_word_t;

/* Different modulation types */
typedef enum radio_modulation_e {
    RADIO_MOD_FSK2  = 0,
    RADIO_MOD_GFSK  = 1,
    RADIO_MOD_OOK   = 3,
    RADIO_MOD_FSK4  = 4,
    RADIO_MOD_MSK   = 7,
    NUM_RADIO_MOD
} radio_modulation_t;

/* Available data rates */
typedef enum rate_e {
    RATE_50,
    RATE_110,
    RATE_300,
    RATE_600,
    RATE_1200,
    RATE_2400,
    RATE_4800,
    RATE_9600,
    RATE_14400,
    RATE_19200,
    RATE_28800,
    RATE_38400,
    RATE_57600,
    RATE_76800,
    RATE_115200,
    NUM_RATE
} rate_t;

/* Radio mode */
typedef enum radio_mode_e
{
    RADIOMODE_NONE = 0,
    RADIOMODE_RX,
    RADIOMODE_TX,
    NUM_RADIOMODE
} radio_mode_t;

/* Radio parameters */
typedef struct radio_parms_s
{
    uint32_t           f_xtal;        // Crystal frequency (Hz)
    float              freq_hz;       // RF frequency;
    float              f_if;          // IF frequency (Hz)
    uint8_t            packet_length; // Packet length if fixed
    radio_modulation_t modulation;    // Type of modulation
    rate_t             drate;         // Data rate of the system
    float              mod_index;     // Modulation index Carlson rule
    uint8_t            fec;           // FEC is in use
    uint8_t            whitening;     // Whitening useds
    preamble_t         preamble;      // Preamble count
    sync_word_t        sync_ctl;      // Sync word control
    float              deviat_factor; // FSK-2 deviation is +/- data rate divised by this factor
    uint32_t           freq_word;     // Frequency 24 bit word FREQ[23..0]
    uint8_t            chanspc_m;     // Channel spacing mantissa 
    uint8_t            chanspc_e;     // Channel spacing exponent
    uint8_t            if_word;       // Intermediate frequency 5 bit word FREQ_IF[4:0] 
    uint8_t            drate_m;       // Data rate mantissa
    uint8_t            drate_e;       // Data rate exponent
    uint8_t            chanbw_m;      // Channel bandwidth mantissa
    uint8_t            chanbw_e;      // Channel bandwidth exponent
    uint8_t            deviat_m;      // Deviation mantissa
    uint8_t            deviat_e;      // Deviation exponent
} radio_parms_t;

/* Handler for radio interrupt data */
typedef volatile struct radio_int_data_s 
{
    spi_parms_t     *spi_parms;
    radio_parms_t   *radio_parms;
    radio_mode_t    mode;                   // Radio mode (essentially Rx or Tx)
    uint32_t        packet_rx_count;        // Number of packets received since put into action
    uint32_t        packet_tx_count;        // Number of packets sent since put into action
    uint8_t         tx_buf[CC11xx_PACKET_COUNT_SIZE]; // Tx buffer
    uint8_t         tx_count;               // Number of bytes in Tx buffer
    uint8_t         rx_buf[CC11xx_PACKET_COUNT_SIZE]; // Rx buffer
    uint8_t         rx_count;               // Number of bytes in Rx buffer
    uint8_t         bytes_remaining;        // Bytes remaining to be read from or written to buffer (composite mode)
    uint8_t         byte_index;             // Current byte index in buffer
    uint8_t         packet_receive;         // Indicates reception of a packet is in progress
    uint8_t         packet_send;            // Indicates transmission of a packet is in progress
} radio_int_data_t;

float       rssi_dbm(uint8_t rssi_dec);

uint32_t    get_freq_word(uint32_t freq_xtal, uint32_t freq_hz);
uint32_t    get_if_word(uint32_t freq_xtal, uint32_t if_hz);
uint8_t     get_mod_word(radio_modulation_t modulation_code);
void        get_chanbw_words(float bw, radio_parms_t *radio_parms);
void        get_rate_words(rate_t data_rate, float mod_index, radio_parms_t *radio_parms);

void        wait_for_state(spi_parms_t *spi_parms, CC11xx_state_t state, uint32_t timeout);

void        radio_turn_idle(spi_parms_t *spi_parms);
/* Those 2 functions used for putting CC1101 in RX mode */
void        radio_turn_rx(spi_parms_t *spi_parms);
void        radio_init_rx(spi_parms_t *spi_parms, radio_parms_t * radio_parms);

void        radio_flush_fifos(spi_parms_t *spi_parms);

int         radio_set_packet_length(spi_parms_t *spi_parms, uint8_t pkt_len);

uint8_t     radio_get_packet_length(spi_parms_t *spi_parms);    
float       radio_get_rate(radio_parms_t *radio_parms);

/* Used to send a packet with CCA */
void        radio_send_packet(spi_parms_t *spi_parms, radio_parms_t * radio_parms, uint8_t *packet, uint8_t size);

void        enable_isr_routine(spi_parms_t *spi_parms, radio_parms_t * radio_parms);

#endif










