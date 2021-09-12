/*
 * Control.c
 *
 *  Created on: Sep 11, 2021
 *      Author: evgenijivlicev
 */
#include "main.h"
#include "queue.h"
#include "ADG2128.h"
#include "usbd_cdc_if.h"

#define EN_USB_PORT GPIOB

#define EN1_PIN     GPIO_PIN_0
#define EN2_PIN     GPIO_PIN_1
#define EN3_PIN     GPIO_PIN_4
#define EN4_PIN     GPIO_PIN_5
#define EN5_PIN     GPIO_PIN_6
#define EN6_PIN     GPIO_PIN_7

#define BRATH_PORT    GPIOC
#define USB_DET_PORT  GPIOC

#define DET1_PIN    GPIO_PIN_0
#define DET2_PIN    GPIO_PIN_1
#define DET3_PIN    GPIO_PIN_2
#define DET4_PIN    GPIO_PIN_3
#define DET5_PIN    GPIO_PIN_4
#define DET6_PIN    GPIO_PIN_5

#define ON_2K_PIN   GPIO_PIN_6
#define ON_1K_PIN   GPIO_PIN_7
#define ON_20_PIN   GPIO_PIN_8
#define OFF_025_PIN GPIO_PIN_9
#define ON_025_PIN  GPIO_PIN_10
#define OFF_1_PIN   GPIO_PIN_11
#define ON_1_PIN    GPIO_PIN_12
#define OFF_10_PIN  GPIO_PIN_13
#define ON_10_PIN   GPIO_PIN_14

static bool EN_025_GEN = false;
static bool EN_1_GEN = false;
static bool EN_10_GEN = false;

static uint8_t breath_count = 0;

extern TIM_HandleTypeDef htim2;

static bool mux1_select = false;
static bool mux2_select = false;

static uint8_t addr = I2C_ADDRESS_AD1;

static uint8_t _usb_conn_state = 0;

//****************************
//Command initialization
//****************************
void CmdInit(void)
{
    GPIO_InitTypeDef  gpioinitstruct = {0};

    /* Configure the GPIO pin */
    gpioinitstruct.Pin    = EN1_PIN | EN2_PIN | EN3_PIN | EN4_PIN | EN5_PIN | EN6_PIN;
    gpioinitstruct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpioinitstruct.Pull   = GPIO_NOPULL;
    gpioinitstruct.Speed  = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(EN_USB_PORT, &gpioinitstruct);

    HAL_GPIO_WritePin(EN_USB_PORT, EN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN_USB_PORT, EN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN_USB_PORT, EN3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN_USB_PORT, EN4_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN_USB_PORT, EN5_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EN_USB_PORT, EN6_PIN, GPIO_PIN_RESET);

    /* Configure the GPIO pin */
    gpioinitstruct.Pin    = ON_2K_PIN | ON_1K_PIN | ON_20_PIN | OFF_025_PIN | ON_025_PIN | OFF_1_PIN | ON_1_PIN | OFF_10_PIN | ON_10_PIN;
    gpioinitstruct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpioinitstruct.Pull   = GPIO_NOPULL;
    gpioinitstruct.Speed  = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(BRATH_PORT, &gpioinitstruct);
    HAL_GPIO_WritePin(BRATH_PORT, ON_2K_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BRATH_PORT, ON_1K_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BRATH_PORT, ON_20_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(BRATH_PORT, OFF_025_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BRATH_PORT, ON_025_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(BRATH_PORT, OFF_1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BRATH_PORT, ON_1_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(BRATH_PORT, OFF_10_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BRATH_PORT, ON_10_PIN, GPIO_PIN_RESET);

    EN_025_GEN = false;
    EN_1_GEN = false;
    EN_10_GEN = false;
}

//****************
//Command handler
//****************
int CmdDataSelect(QUEUE_TypeDef *q)
{
	uint16_t freq = 1;
	TIM_OC_InitTypeDef sConfigOC = {0};
	uint8_t buffer[4] = {0};

	switch(q->cmd)
	{
	case USB_POWER_EN:
        if ((q->buffer & 0x01) == 0x01)
        	HAL_GPIO_WritePin(EN_USB_PORT, EN1_PIN, GPIO_PIN_SET);
        else
        	HAL_GPIO_WritePin(EN_USB_PORT, EN1_PIN, GPIO_PIN_RESET);

        if ((q->buffer & 0x02) == 0x02)
        	HAL_GPIO_WritePin(EN_USB_PORT, EN2_PIN, GPIO_PIN_SET);
        else
        	HAL_GPIO_WritePin(EN_USB_PORT, EN2_PIN, GPIO_PIN_RESET);

        if ((q->buffer & 0x04) == 0x04)
        	HAL_GPIO_WritePin(EN_USB_PORT, EN3_PIN, GPIO_PIN_SET);
        else
        	HAL_GPIO_WritePin(EN_USB_PORT, EN3_PIN, GPIO_PIN_RESET);

        if ((q->buffer & 0x08) == 0x08)
        	HAL_GPIO_WritePin(EN_USB_PORT, EN4_PIN, GPIO_PIN_SET);
        else
        	HAL_GPIO_WritePin(EN_USB_PORT, EN4_PIN, GPIO_PIN_RESET);

        if ((q->buffer & 0x10) == 0x10)
        	HAL_GPIO_WritePin(EN_USB_PORT, EN5_PIN, GPIO_PIN_SET);
        else
        	HAL_GPIO_WritePin(EN_USB_PORT, EN5_PIN, GPIO_PIN_RESET);

        if ((q->buffer & 0x20) == 0x20) {
        	HAL_GPIO_WritePin(EN_USB_PORT, EN6_PIN, GPIO_PIN_SET);
        }
        else {
        	HAL_GPIO_WritePin(EN_USB_PORT, EN6_PIN, GPIO_PIN_RESET);
        }
		break;
	case USB_DETECT:
		_usb_conn_state = 0;

		if (HAL_GPIO_ReadPin(USB_DET_PORT, DET1_PIN) == GPIO_PIN_SET)
			_usb_conn_state |= 0x01;

		if (HAL_GPIO_ReadPin(USB_DET_PORT, DET2_PIN) == GPIO_PIN_SET)
			_usb_conn_state |= 0x02;

		if (HAL_GPIO_ReadPin(USB_DET_PORT, DET3_PIN) == GPIO_PIN_SET)
			_usb_conn_state |= 0x04;

		if (HAL_GPIO_ReadPin(USB_DET_PORT, DET4_PIN) == GPIO_PIN_SET)
			_usb_conn_state |= 0x08;

		if (HAL_GPIO_ReadPin(USB_DET_PORT, DET5_PIN) == GPIO_PIN_SET)
			_usb_conn_state |= 0x10;

		if (HAL_GPIO_ReadPin(USB_DET_PORT, DET6_PIN) == GPIO_PIN_SET)
			_usb_conn_state |= 0x20;

		buffer[0] = (uint8_t)USB_DETECT;
		buffer[1] = _usb_conn_state;
		buffer[2] = 0x00;
		buffer[3] = buffer[0] ^ buffer[1] ^ buffer[2]; //crc

		CDC_Transmit_FS(buffer, 4);

		break;
	case BREATH_MODE:
		if ((q->buffer & 0x01) == 0x01)
			HAL_GPIO_WritePin(BRATH_PORT, ON_2K_PIN, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(BRATH_PORT, ON_2K_PIN, GPIO_PIN_RESET);

		if ((q->buffer & 0x02) == 0x02)
			HAL_GPIO_WritePin(BRATH_PORT, ON_1K_PIN, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(BRATH_PORT, ON_1K_PIN, GPIO_PIN_RESET);

		if ((q->buffer & 0x04) == 0x04)
			HAL_GPIO_WritePin(BRATH_PORT, ON_20_PIN, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(BRATH_PORT, ON_20_PIN, GPIO_PIN_RESET);

		if ((q->buffer & 0x08) == 0x08)
			EN_025_GEN = true;
		else
			EN_025_GEN = false;

		if ((q->buffer & 0x10) == 0x10)
			EN_1_GEN = true;
		else
			EN_1_GEN = false;

		if ((q->buffer & 0x20) == 0x20)
			EN_10_GEN = true;
		else
			EN_10_GEN = false;
		break;
	case BREATH_PERIOD:
		freq  = q->buffer;
		HAL_SYSTICK_Config(SystemCoreClock / (1000 / freq)); //1=0.5kHz/16, 10=50Hz/16, 100=5Hz/16
		break;
	case BREATH_GEN1:
		/* Stop channel 2 */
		if (HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2) != HAL_OK)
		{
			/* PWM Generation Error */
			Error_Handler();
		}

		  sConfigOC.OCMode = TIM_OCMODE_PWM1;
		  sConfigOC.Pulse = (uint32_t)q->buffer;
		  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
		  {
		    Error_Handler();
		  }

		/* Start channel 2 */
		if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2) != HAL_OK)
		{
			/* PWM Generation Error */
			Error_Handler();
		}
		break;
	case BREATH_GEN2:
		/* Stop channel 3 */
		if (HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3) != HAL_OK)
		{
			/* PWM Generation Error */
			Error_Handler();
		}

		  sConfigOC.OCMode = TIM_OCMODE_PWM1;
		  sConfigOC.Pulse = (uint32_t)q->buffer;
		  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
		  {
		    Error_Handler();
		  }

		/* Start channel 3 */
		if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK)
		{
			/* PWM Generation Error */
			Error_Handler();
		}
		break;
	case MUX_AD1:
		if (mux1_select == false)
		{
			addr = I2C_ADDRESS_AD1;
			ADG2128_init(&addr);
			mux1_select = true;
			mux2_select = false;
		}
		setRoute((uint8_t)((q->buffer & 0xFF00) >> 8), (uint8_t)(q->buffer & 0x00FF), false);
		break;
	case MUX_AD2:
		if (mux2_select == false)
		{
			addr = I2C_ADDRESS_AD2;
			ADG2128_init(&addr);
			mux2_select = true;
			mux1_select = false;
		}
		setRoute((uint8_t)((q->buffer & 0xFF00) >> 8), (uint8_t)(q->buffer & 0x00FF), false);
		break;
	default:
		break;
	}
	return 0;
}

//****************************
//Breath control wave handler
//****************************
void BreathTimer(void)
{
	if (EN_025_GEN)
	{
		if ((breath_count & 0x10) == 0x10)
		{
		    HAL_GPIO_WritePin(BRATH_PORT, OFF_025_PIN, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(BRATH_PORT, ON_025_PIN, GPIO_PIN_RESET);
		}
		else
		{
			HAL_GPIO_WritePin(BRATH_PORT, ON_025_PIN, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(BRATH_PORT, OFF_025_PIN, GPIO_PIN_RESET);
		}
	}
	else
	{
		HAL_GPIO_WritePin(BRATH_PORT, ON_025_PIN, GPIO_PIN_RESET);
	    HAL_GPIO_WritePin(BRATH_PORT, OFF_025_PIN, GPIO_PIN_RESET);
	}

	if (EN_1_GEN)
	{
		if ((breath_count & 0x10) == 0x10)
		{
		    HAL_GPIO_WritePin(BRATH_PORT, OFF_1_PIN, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(BRATH_PORT, ON_1_PIN, GPIO_PIN_RESET);
		}
		else
		{
			HAL_GPIO_WritePin(BRATH_PORT, ON_1_PIN, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(BRATH_PORT, OFF_1_PIN, GPIO_PIN_RESET);
		}
	}
	else
	{
		HAL_GPIO_WritePin(BRATH_PORT, ON_1_PIN, GPIO_PIN_RESET);
	    HAL_GPIO_WritePin(BRATH_PORT, OFF_1_PIN, GPIO_PIN_RESET);
	}

	if (EN_10_GEN)
	{
		if ((breath_count & 0x10) == 0x10)
		{
		    HAL_GPIO_WritePin(BRATH_PORT, OFF_10_PIN, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(BRATH_PORT, ON_10_PIN, GPIO_PIN_RESET);
		}
		else
		{
			HAL_GPIO_WritePin(BRATH_PORT, ON_10_PIN, GPIO_PIN_SET);
		    HAL_GPIO_WritePin(BRATH_PORT, OFF_10_PIN, GPIO_PIN_RESET);
		}
	}
	else
	{
		HAL_GPIO_WritePin(BRATH_PORT, ON_10_PIN, GPIO_PIN_RESET);
	    HAL_GPIO_WritePin(BRATH_PORT, OFF_10_PIN, GPIO_PIN_RESET);
	}
	breath_count++;
}




