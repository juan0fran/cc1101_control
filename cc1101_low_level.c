#include "cc1101_low_level.h"
#include "cc1101_defines.h"
#include <stdio.h>

int spi_transfer(uint8_t * t, uint8_t * r, uint8_t l)
{
	int i = 0;
	printf("SPI Transfer: ");
	for (i = 0; i < l - 1; i++){
		printf("0x%02X, ", t[i]);
	}
	printf("0x%02X\n", t[i]);
	return 0;
}

#define SPI_TRANSFER(x,y,z)  spi_transfer (x, y, z)


int CC_SPIInit(spi_parms_t *spi_parms)
{

	return 0;
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
	printf("Writting %02X to %02X\n", byte, addr);
	return 0;
}

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

	return 0;
}

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
	return 0;
}

int  CC_PowerupResetCCxxxx(spi_parms_t *spi_parms)
{
	return CC_SPIStrobe(spi_parms, CC11xx_SRES);
}

void disable_IT(void)
{


}

void enable_IT(void)
{


}