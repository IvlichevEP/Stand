/*
 * I2C.c
 *
 *  Created on: Sep 7, 2021
 *      Author: evgenijivlicev
 */

#include "main.h"
#include "I2C.h"

/* Private define ------------------------------------------------------------*/
/* Uncomment this line to use the board as master, if not it is used as slave */
#define MASTER_BOARD

/* I2C handler declaration */
extern I2C_HandleTypeDef hi2c1;


/* Buffer used for transmission */
uint8_t aTxBuffer[] = " ****I2C_TwoBoards communication based on Polling**** ";

/* Buffer used for reception */
static uint8_t aRxBuffer[RXBUFFERSIZE];

static uint8_t transfer_counter = 0;

#ifdef MASTER_BOARD

HAL_StatusTypeDef I2C_Write(uint8_t address, uint8_t* txBuffer, uint16_t length) {
	if (length > TXBUFFERSIZE) {
		return HAL_ERROR;
	}

	  /*##-2- Start the transmission process #####################################*/
	  /* While the I2C in reception process, user can transmit data through
	     "aTxBuffer" buffer */
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)address, txBuffer, length, 1000);
//	  while(HAL_I2C_Master_Seq_Transmit_IT(&hi2c1, (uint16_t)address, txBuffer, length, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
//	  {
//	    /* Error_Handler() function is called when Timeout error occurs.
//	       When Acknowledge failure occurs (Slave don't acknowledge it's address)
//	       Master restarts communication */
//	    if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF)
//	    {
//	      Error_Handler();
//	    }
//	  }
	return HAL_OK;
}

HAL_StatusTypeDef I2C_Read(uint8_t address, uint8_t txData, uint8_t* rxBuffer, uint16_t length) {
	volatile uint32_t error;
	if (length > RXBUFFERSIZE) {
		return HAL_ERROR;
	}

	uint8_t txBuffer[] = {txData};


	/*##-2- Start the transmission process #####################################*/
	/* While the I2C in reception process, user can transmit data through
	     "aTxBuffer" buffer */
	while(HAL_I2C_Master_Seq_Transmit_IT(&hi2c1, (uint16_t)address, txBuffer, /*COUNTOF(txBuffer)*/0x01, I2C_FIRST_FRAME)!= HAL_OK)
	{
		/* Error_Handler() function is called when Timeout error occurs.
	       When Acknowledge failure occurs (Slave don't acknowledge it's address)
	       Master restarts communication */
		error = HAL_I2C_GetError(&hi2c1);
		if (error != HAL_I2C_ERROR_AF)
		{
			Error_Handler();
		}
	}

	/*##-3- Wait for the end of the transfer ###################################*/
	/*  Before starting a new communication transfer, you need to check the current
	      state of the peripheral; if itís busy you need to wait for the end of current
	      transfer before starting a new one.
	      For simplicity reasons, this example is just waiting till the end of the
	      transfer, but application may perform other tasks while transfer operation
	      is ongoing. */
	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
	}

	/*##-4- Put I2C peripheral in reception process ############################*/
	while(HAL_I2C_Master_Seq_Receive_IT(&hi2c1, (uint16_t)address, (uint8_t *)aRxBuffer, length, I2C_LAST_FRAME) != HAL_OK)
	{
		/* Error_Handler() function is called when Timeout error occurs.
	       When Acknowledge failure occurs (Slave don't acknowledge it's address)
	       Master restarts communication */
		error = HAL_I2C_GetError(&hi2c1);
		if (error != HAL_I2C_ERROR_AF)
		{
			Error_Handler();
		}
	}

	for (uint16_t i = 0; i < length; i++) {
		rxBuffer[i] = aRxBuffer[i];
	}

	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
	}

	return HAL_OK;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
	transfer_counter++;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
	transfer_counter++;
}

#endif



