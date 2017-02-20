#ifndef __CC1101_LOW_LEVEL_H__
#define __CC1101_LOW_LEVEL_H__

#include <stdint.h>

#define CC11xx_PIN(x) usleep(x)

#define GDO0_pin 1
#define GDO2_pin 0

/* spi structure */
typedef struct spi_parms_s
{
    int      fd;
    int      ret; 	 /* Ret value of funcking SPI */
    uint8_t  status;
    uint8_t  tx[65]; // max 1 command byte + 64 bytes FIFO
    uint8_t  rx[65]; // max 1 status byte + 64 bytes FIFO
    uint8_t  len;
} spi_parms_t;


int 	CC_SPIInit(spi_parms_t *spi_parms);
int  	CC_SPIWriteReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t byte);
int  	CC_SPIWriteBurstReg(spi_parms_t *spi_parms, uint8_t addr, const uint8_t *bytes, uint8_t count);
int  	CC_SPIReadReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t *byte);
int  	CC_SPIReadBurstReg(spi_parms_t *spi_parms, uint8_t addr, uint8_t **bytes, uint8_t count);
int  	CC_SPIReadStatus(spi_parms_t *spi_parms, uint8_t addr, uint8_t *status);
int  	CC_SPIStrobe(spi_parms_t *spi_parms, uint8_t strobe);
int  	CC_PowerupResetCCxxxx(spi_parms_t *spi_parms);
void 	disable_IT(void);
void 	enable_IT(void);

#endif
