/*
 * ADG2128.h
 *
 *  Created on: Sep 7, 2021
 *      Author: evgenijivlicev
 */

#ifndef APPLICATION_USER_ADG2128_H_
#define APPLICATION_USER_ADG2128_H_

#include "stdbool.h"
#include "main.h"

#define RST1_PIN                      GPIO_PIN_10
#define RST2_PIN                      GPIO_PIN_11
#define RST12_GPIO_PORT                GPIOB
#define RST12_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOB_CLK_ENABLE()
#define RST12_GPIO_CLK_DISABLE()      __HAL_RCC_GPIOB_CLK_DISABLE()

#define RESET_PIN                   10
#define ADG2128_DEFAULT_I2C_ADDR    0x70 << 1

#define ADG2128_SERIALIZE_VERSION   0x01
#define ADG2128_SERIALIZE_SIZE        17

/* Class flags. */
#define ADG2128_FLAG_INITIALIZED    0x0001
#define ADG2128_FLAG_ALLOW_MR_TO_C  0x0002
#define ADG2128_FLAG_ALLOW_R_TO_MC  0x0004
#define ADG2128_FLAG_PRESERVE_STATE 0x0008
#define ADG2128_FLAG_PINS_CONFD     0x0010
#define ADG2128_FLAG_FROM_BLOB      0x0020

#define ADG2128_FLAG_SERIAL_MASK    0x000E  // Only these bits are serialized.

typedef enum {
  NO_ERROR           = 0,   // There was no error.
  ABSENT             = -1,  // The ADG2128 appears to not be connected to the bus.
  BUS                = -2,  // Something went wrong with the i2c bus.
  BAD_COLUMN         = -3,  // Column was out-of-bounds.
  BAD_ROW            = -4   // Row was out-of-bounds.
} ADG2128_ERROR;

typedef uint8_t TwoWire;

ADG2128_ERROR ADG2128_init(TwoWire* b);
ADG2128_ERROR ADG2128_reset();
ADG2128_ERROR setRoute(uint8_t col, uint8_t row, bool defer);
ADG2128_ERROR unsetRoute(uint8_t col, uint8_t row, bool defer);
ADG2128_ERROR refresh();

extern uint16_t _flags; // Class flags.

#endif /* APPLICATION_USER_ADG2128_H_ */
