#ifndef __CC1101_WRAPPER_H__
#define __CC1101_WRAPPER_H__

#include "stm32l4xx_hal.h"
#include "spi.h"

#define MSLEEP(x) HAL_Delay(x)
#define MDELAY(x) MSLEEP(x)
#define SPI_TRANSFER(x, y, z)  spi_transfer(x, y, z)

#define CC11xx_GDO0()	HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0)
#define CC11xx_GDO2()	HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)

#endif
