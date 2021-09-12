/*
 * I2C.h
 *
 *  Created on: Sep 7, 2021
 *      Author: evgenijivlicev
 */

#ifndef APPLICATION_USER_I2C_H_
#define APPLICATION_USER_I2C_H_

#include "main.h"

/* Definition for I2Cx clock resources */
#define I2Cx                            I2C1

extern uint8_t aTxBuffer[];

/* Exported macro ------------------------------------------------------------*/
#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))
/* Exported functions ------------------------------------------------------- */

/* Size of Transmission buffer */
#define TXBUFFERSIZE                      (COUNTOF(aTxBuffer) - 1)
/* Size of Reception buffer */
#define RXBUFFERSIZE                      TXBUFFERSIZE

void I2C_Init(I2C_HandleTypeDef* hi2cx);
HAL_StatusTypeDef I2C_Write(uint8_t address, uint8_t* txBuffer, uint16_t length);
HAL_StatusTypeDef I2C_Read(uint8_t address, uint8_t txData, uint8_t* rxBuffer, uint16_t length);

#endif /* APPLICATION_USER_I2C_H_ */
