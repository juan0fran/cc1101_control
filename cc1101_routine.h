#ifndef __CC1101_DEFINES_H__
#define __CC1101_DEFINES_H__

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

typedef enum CC11xx_state_e {
    CC11xx_STATE_SLEEP = 0,
    CC11xx_STATE_IDLE,
    CC11xx_STATE_XOFF,
    CC11xx_STATE_VCOON_MC,
    CC11xx_STATE_REGON_MC,
    CC11xx_STATE_MANCAL,
    CC11xx_STATE_VCOON,
    CC11xx_STATE_REGON,
    CC11xx_STATE_STARTCAL,
    CC11xx_STATE_BWBOOST,
    CC11xx_STATE_FS_LOCK,
    CC11xx_STATE_IFADCON,
    CC11xx_STATE_ENDCAL,
    CC11xx_STATE_RX,
    CC11xx_STATE_RX_END,
    CC11xx_STATE_RX_RST,
    CC11xx_STATE_TXRX_SWITCH,
    CC11xx_STATE_RXFIFO_OVERFLOW,
    CC11xx_STATE_FSTXON,
    CC11xx_STATE_TX,
    CC11xx_STATE_TX_END,
    CC11xx_STATE_RXTX_SWITCH,
    CC11xx_STATE_TXFIFO_UNDERFLOW
} CC11xx_state_t;

// Configuration Registers
#define CC11xx_IOCFG2       0x00        // GDO2 output pin configuration
#define CC11xx_IOCFG1       0x01        // GDO1 output pin configuration
#define CC11xx_IOCFG0       0x02        // GDO0 output pin configuration
#define CC11xx_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CC11xx_SYNC1        0x04        // Sync word, high byte
#define CC11xx_SYNC0        0x05        // Sync word, low byte
#define CC11xx_PKTLEN       0x06        // Packet length
#define CC11xx_PKTCTRL1     0x07        // Packet automation control
#define CC11xx_PKTCTRL0     0x08        // Packet automation control
#define CC11xx_ADDR         0x09        // Device address
#define CC11xx_CHANNR       0x0A        // Channel number
#define CC11xx_FSCTRL1      0x0B        // Frequency synthesizer control
#define CC11xx_FSCTRL0      0x0C        // Frequency synthesizer control
#define CC11xx_FREQ2        0x0D        // Frequency control word, high byte
#define CC11xx_FREQ1        0x0E        // Frequency control word, middle byte
#define CC11xx_FREQ0        0x0F        // Frequency control word, low byte
#define CC11xx_MDMCFG4      0x10        // Modem configuration
#define CC11xx_MDMCFG3      0x11        // Modem configuration
#define CC11xx_MDMCFG2      0x12        // Modem configuration
#define CC11xx_MDMCFG1      0x13        // Modem configuration
#define CC11xx_MDMCFG0      0x14        // Modem configuration
#define CC11xx_DEVIATN      0x15        // Modem deviation setting
#define CC11xx_MCSM2        0x16        // Main Radio Cntrl State Machine config
#define CC11xx_MCSM1        0x17        // Main Radio Cntrl State Machine config
#define CC11xx_MCSM0        0x18        // Main Radio Cntrl State Machine config
#define CC11xx_FOCCFG       0x19        // Frequency Offset Compensation config
#define CC11xx_BSCFG        0x1A        // Bit Synchronization configuration
#define CC11xx_AGCCTRL2     0x1B        // AGC control
#define CC11xx_AGCCTRL1     0x1C        // AGC control
#define CC11xx_AGCCTRL0     0x1D        // AGC control
#define CC11xx_WOREVT1      0x1E        // High byte Event 0 timeout
#define CC11xx_WOREVT0      0x1F        // Low byte Event 0 timeout
#define CC11xx_WORCTRL      0x20        // Wake On Radio control
#define CC11xx_FREND1       0x21        // Front end RX configuration
#define CC11xx_FREND0       0x22        // Front end TX configuration
#define CC11xx_FSCAL3       0x23        // Frequency synthesizer calibration
#define CC11xx_FSCAL2       0x24        // Frequency synthesizer calibration
#define CC11xx_FSCAL1       0x25        // Frequency synthesizer calibration
#define CC11xx_FSCAL0       0x26        // Frequency synthesizer calibration
#define CC11xx_RCCTRL1      0x27        // RC oscillator configuration
#define CC11xx_RCCTRL0      0x28        // RC oscillator configuration
#define CC11xx_FSTEST       0x29        // Frequency synthesizer cal control
#define CC11xx_PTEST        0x2A        // Production test
#define CC11xx_AGCTEST      0x2B        // AGC test
#define CC11xx_TEST2        0x2C        // Various test settings
#define CC11xx_TEST1        0x2D        // Various test settings
#define CC11xx_TEST0        0x2E        // Various test settings

// Strobe commands
#define CC11xx_SRES         0x30        // Reset chip.
#define CC11xx_SFSTXON      0x31        // Enable/calibrate freq synthesizer
#define CC11xx_SXOFF        0x32        // Turn off crystal oscillator.
#define CC11xx_SCAL         0x33        // Calibrate freq synthesizer & disable
#define CC11xx_SRX          0x34        // Enable RX.
#define CC11xx_STX          0x35        // Enable TX.
#define CC11xx_SIDLE        0x36        // Exit RX / TX
#define CC11xx_SAFC         0x37        // AFC adjustment of freq synthesizer
#define CC11xx_SWOR         0x38        // Start automatic RX polling sequence
#define CC11xx_SPWD         0x39        // Enter pwr down mode when CSn goes hi
#define CC11xx_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CC11xx_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CC11xx_SWORRST      0x3C        // Reset real time clock.
#define CC11xx_SNOP         0x3D        // No operation.

// Status registers
#define CC11xx_PARTNUM      0x30        // Part number
#define CC11xx_VERSION      0x31        // Current version number
#define CC11xx_FREQEST      0x32        // Frequency offset estimate
#define CC11xx_LQI          0x33        // Demodulator estimate for link quality
#define CC11xx_RSSI         0x34        // Received signal strength indication
#define CC11xx_MARCSTATE    0x35        // Control state machine state
#define CC11xx_WORTIME1     0x36        // High byte of WOR timer
#define CC11xx_WORTIME0     0x37        // Low byte of WOR timer
#define CC11xx_PKTSTATUS    0x38        // Current GDOx status and packet status
#define CC11xx_VCO_VC_DAC   0x39        // Current setting from PLL cal module
#define CC11xx_TXBYTES      0x3A        // Underflow and # of bytes in TXFIFO
#define CC11xx_RXBYTES      0x3B        // Overflow and # of bytes in RXFIFO
#define CC11xx_NUM_RXBYTES  0x7F        // Mask "# of bytes" field in _RXBYTES

// Other memory locations
#define CC11xx_PATABLE      0x3E
#define CC11xx_TXFIFO       0x3F
#define CC11xx_RXFIFO       0x3F

// Masks for appended status bytes
#define CC11xx_LQI_RX       0x01        // Position of LQI byte
#define CC11xx_CRC_OK       0x80        // Mask "CRC_OK" bit within LQI byte

// Definitions to support burst/single access:
#define CC11xx_WRITE_BURST  0x40
#define CC11xx_READ_SINGLE  0x80
#define CC11xx_READ_BURST   0xC0

// Various constants
#define CC11xx_FIFO_SIZE         64     // Rx or Tx FIFO size
#define CC11xx_PACKET_COUNT_SIZE 255    // Packet bytes maximum count


/* spi structure */
typedef struct spi_parms_s
{
    int      fd;
    int      ret;    /* Ret value of funcking SPI */
    uint8_t  status;
    uint8_t  tx[65]; // max 1 command byte + 64 bytes FIFO
    uint8_t  rx[65]; // max 1 status byte + 64 bytes FIFO
    uint8_t  len;
} spi_parms_t;

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



int     CC_SPIInit(spi_parms_t *spi_parms);
int     CC_SPIWriteReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t byte);
int     CC_SPIWriteBurstReg(spi_parms_t *spi_parms, uint8_t addr, const uint8_t *bytes, uint8_t count);
int     CC_SPIReadReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t *byte);
int     CC_SPIReadBurstReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t **bytes, uint8_t count);
int     CC_SPIReadStatus(spi_parms_t *spi_parms, uint8_t addr, uint8_t *status);
int     CC_SPIStrobe(spi_parms_t *spi_parms, uint8_t strobe);
int     CC_PowerupResetCCxxxx(spi_parms_t *spi_parms);
int     CC11xx_GDO2(void);
int     CC11xx_GDO0(void);
void    disable_IT(void);
void    enable_IT(void);

int set_radio_parameters(   float freq_hz, float freq_if, 
                            radio_modulation_t mod, rate_t data_rate, float mod_index,
                            uint8_t packet_length, uint8_t fec, uint8_t white,      
                            preamble_t preamble, sync_word_t sync_word,
                            radio_parms_t * radio_parms);

int init_radio_config(spi_parms_t * spi_parms, radio_parms_t * radio_parms);

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
