/*
 * ADG2128.c
 *
 *  Created on: Sep 7, 2021
 *      Author: evgenijivlicev
 */

#include "ADG2128.h"
#include "I2C.h"

static uint8_t _ADDR;       // The device address on the i2c bus
static uint8_t _RESET_PIN;  // ADG2128 reset pin
uint16_t _flags = 0;        // Class flags.
TwoWire* _bus   = NULL;
static uint8_t _values[12];

static void ADG2128(const uint8_t i2c_addr, const uint8_t r_pin);
static ADG2128_ERROR compose_first_byte(uint8_t col, uint8_t row, bool set, uint8_t* result);
static int8_t _ll_pin_init();
static int8_t _write_device(uint8_t row, uint8_t conn);
static int8_t _read_device();
static void delay_adg(uint8_t timeout);

static ADG2128_ERROR changeRoute(uint8_t col, uint8_t row, bool sw_closed, bool defer);
//static uint8_t getCols(uint8_t row);
//static uint16_t getRows(uint8_t col);

static const uint8_t readback_addr[12] = {
  0x34, 0x3c, 0x74, 0x7c, 0x35, 0x3d, 0x75, 0x7d, 0x36, 0x3e, 0x76, 0x7e
};

//static const char* errorToStr(ADG2128_ERROR err) {
//  switch (err) {
//    case (ADG2128_ERROR)NO_ERROR:    return "NO_ERROR";
//    case (ADG2128_ERROR)ABSENT:      return "ABSENT";
//    case (ADG2128_ERROR)BUS:         return "BUS";
//    case (ADG2128_ERROR)BAD_COLUMN:  return "BAD_COLUMN";
//    case (ADG2128_ERROR)BAD_ROW:     return "BAD_ROW";
//    default:                         return "UNKNOWN";
//  }
//}

/* Private functions --------------------------------------------------------- */
/* Flag manipulation inlines */
// static uint16_t _adg_flags     (              ) { return _flags;           };
static bool     _adg_flag      (uint16_t _flag) { return (_flags & _flag); };
static void     _adg_clear_flag(uint16_t _flag) { _flags &= ~_flag;        };
static void     _adg_set_flag  (uint16_t _flag) { _flags |= _flag;         };
// static void _adg_set_flag(uint16_t _flag, bool nu) {
//   if (nu) _flags |= _flag;
//   else    _flags &= ~_flag;
// };

static bool _from_blob()  { return _adg_flag(ADG2128_FLAG_FROM_BLOB);   };
static bool initialized() { return _adg_flag(ADG2128_FLAG_INITIALIZED); };
static bool preserveOnDestroy() {
	return _adg_flag(ADG2128_FLAG_PRESERVE_STATE);
};
// static void preserveOnDestroy(bool x) {
//  _adg_set_flag(ADG2128_FLAG_PRESERVE_STATE, x);
//};

/*
* Constructor.
*/
static void ADG2128(const uint8_t i2c_addr, const uint8_t r_pin) {
	_ADDR = i2c_addr;
	_RESET_PIN = r_pin;
	for (uint8_t i = 0; i < 12; i++) {
		_values[i] = 0;
	}
}

/*
*
*/
ADG2128_ERROR ADG2128_init(TwoWire* b) {
  ADG2128(((uint8_t)b[0]) << 1, RESET_PIN);

  _adg_clear_flag(ADG2128_FLAG_INITIALIZED);
  if (!_adg_flag(ADG2128_FLAG_PINS_CONFD)) {
    _ll_pin_init();
  }
  if (NULL != b) {
    _bus = b;
  }

  if (_from_blob()) {
    // Copy the blob-imparted values and clear the flag so we don't do this again.
    _adg_clear_flag(ADG2128_FLAG_FROM_BLOB);
    uint8_t vals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (uint8_t i = 0; i < 12; i++) {  vals[i] = _values[i];  }
    for (uint8_t i = 0; i < 12; i++) {
      uint8_t row_val = vals[i];
      for (uint8_t j = 0; j < 8; j++) {
        // This will defer switch disconnect until the last write is completed.
        // So if reset fails, the part will be in an indeterminate state, but
        //   nothing will have changed in the switches.
        row_val = row_val >> j;
        if ((ADG2128_ERROR)NO_ERROR != changeRoute(j, i, (row_val & 1), !((11 == i) && (7 == j)))) {
          return (ADG2128_ERROR)BUS;
        }
      }
    }
    _adg_set_flag(ADG2128_FLAG_INITIALIZED);
    return (ADG2128_ERROR)NO_ERROR;
  }
  else {
    if (!preserveOnDestroy()) {
      return ADG2128_reset();
    }
    return (0 == _read_device()) ? (ADG2128_ERROR)NO_ERROR : (ADG2128_ERROR)BUS;
  }
}

/*
* Opens all switches.
* Uses hardware reset if possible. Otherwise, will write each of the 96 switches
*   one by one. This will have a non-trivial time load unless it is re-worked.
*/
ADG2128_ERROR ADG2128_reset() {
  _adg_clear_flag(ADG2128_FLAG_INITIALIZED);
  if (255 != _RESET_PIN) {
	  HAL_GPIO_WritePin(RST12_GPIO_PORT, RST1_PIN, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(RST12_GPIO_PORT, RST2_PIN, GPIO_PIN_RESET);
	  delay_adg(100);
	  HAL_GPIO_WritePin(RST12_GPIO_PORT, RST1_PIN, GPIO_PIN_SET);
	  HAL_GPIO_WritePin(RST12_GPIO_PORT, RST2_PIN, GPIO_PIN_SET);
	  delay_adg(100);
  }
  else {
    for (int i = 0; i < 12; i++) {
      for (int j = 0; j < 8; j++) {
        // This will defer switch disconnect until the last write is completed.
        // So if reset fails, the part will be in an indeterminate state, but
        //   nothing will have changed in the switches.
        if ((ADG2128_ERROR)NO_ERROR != unsetRoute(j, i, !((11 == i) && (7 == j)))) {
          return (ADG2128_ERROR)BUS;
        }
      }
    }
  }
  ADG2128_ERROR ret = (ADG2128_ERROR)BUS;
  if (0 == _read_device()) {
    ret = (ADG2128_ERROR)NO_ERROR;
  }
  return ret;
}

static ADG2128_ERROR compose_first_byte(uint8_t col, uint8_t row, bool set, uint8_t* result) {
  if (col > 7)  return (ADG2128_ERROR)BAD_COLUMN;
  if (row > 11) return (ADG2128_ERROR)BAD_ROW;
  uint8_t temp = row;
  if (temp >= 6) temp = temp + 2; // Dance around the reserved range in the middle.
  *result = (temp << 3) + col + (set ? 0x80 : 0x00);
  return (ADG2128_ERROR)NO_ERROR;
}


ADG2128_ERROR setRoute(uint8_t col, uint8_t row, bool defer) {
  return changeRoute(col, row, true, defer);
}


ADG2128_ERROR unsetRoute(uint8_t col, uint8_t row, bool defer) {
  return changeRoute(col, row, false, defer);
}

/*
* Stores everything about the class in the provided buffer in this format...
*   Offset | Data
*   -------|----------------------
*   0      | Serializer version
*   1      | i2c address
*   2      | Reset pin
*   3      | Flags MSB
*   4      | Flags LSB
*   5-16   | Switch configuration
*
* Returns the number of bytes written to the buffer.
*/
uint8_t serialize(uint8_t* buf, unsigned int len) {
  uint8_t offset = 0;
  if (len >= ADG2128_SERIALIZE_SIZE) {
    if (initialized()) {
      uint16_t f = _flags & ADG2128_FLAG_SERIAL_MASK;
      *(buf + offset++) = ADG2128_SERIALIZE_VERSION;
      *(buf + offset++) = _ADDR;
      *(buf + offset++) = _RESET_PIN;
      *(buf + offset++) = (uint8_t) 0xFF & (f >> 8);
      *(buf + offset++) = (uint8_t) 0xFF & f;
      for (uint8_t i = 0; i < 12; i++) {
        *(buf + offset++) = _values[i];
      }
    }
  }
  return offset;
}


int8_t unserialize(const uint8_t* buf, const unsigned int len) {
  uint8_t offset = 0;
  uint8_t expected_sz = 255;
  if (len >= ADG2128_SERIALIZE_SIZE) {
    uint16_t f = 0;
    uint8_t vals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    switch (*(buf + offset++)) {
      case ADG2128_SERIALIZE_VERSION:
        expected_sz = ADG2128_SERIALIZE_SIZE;
        f = (*(buf + 3) << 8) | *(buf + 4);
        _flags = (_flags & ~ADG2128_FLAG_SERIAL_MASK) | (f & ADG2128_FLAG_SERIAL_MASK);
        offset += 4;  // Skip to the register offset.
        for (uint8_t i = 0; i < 12; i++) {
          vals[i] = *(buf + offset++);
        }
        break;
      default:  // Unhandled serializer version.
        return -1;
    }
    if (initialized()) {
      // If the device has already been initialized, we impart the new conf.
      for (uint8_t i = 0; i < 12; i++) {
        uint8_t row_val = vals[i];
        for (uint8_t j = 0; j < 8; j++) {
          // This will defer switch disconnect until the last write is completed.
          // So if reset fails, the part will be in an indeterminate state, but
          //   nothing will have changed in the switches.
          if ((ADG2128_ERROR)NO_ERROR != changeRoute(j, i, ((row_val >> j) & 1), !((11 == i) && (7 == j)))) {
            return -2;
          }
        }
      }
    }
    else {
      _adg_set_flag(ADG2128_FLAG_FROM_BLOB);
      for (uint8_t i = 0; i < 12; i++) {
        _values[i] = vals[i];   // Save state for init()
      }
    }
  }
  return (expected_sz == offset) ? 0 : -1;
}

ADG2128_ERROR changeRoute(uint8_t col, uint8_t row, bool sw_closed, bool defer) {
  uint8_t temp;
  ADG2128_ERROR return_value = compose_first_byte(col, row, sw_closed, &temp);
  if ((ADG2128_ERROR)NO_ERROR == return_value) {
    return_value = (ADG2128_ERROR)BUS;
    if (0 == _write_device(temp, (defer ? 0 : 1))) {
      _values[row] = (sw_closed) ? (_values[row] | (1 << col)) : (_values[row] & ~(1 << col));
      return_value = (ADG2128_ERROR)NO_ERROR;
    }
  }
  return return_value;
}

ADG2128_ERROR refresh() {
  return (0 != _read_device()) ? (ADG2128_ERROR)BUS : (ADG2128_ERROR)NO_ERROR;
}

/*
* Readback on this part is organized by rows, with the return bits
*   being the state of the switches to the corresponding column.
* The readback address table is hard-coded in the readback_addr array.
*/
static int8_t _read_device() {
	int8_t ret = -3;
	if (NULL != _bus) {
		uint8_t rxBuffer[] = {0x00};
		for (uint8_t row = 0; row < 12; row++) {
			if ((uint8_t)(I2C_Read(_ADDR, readback_addr[row], rxBuffer, /*COUNTOF(rxBuffer)*/0x01)) == 0x00) {
				_values[row] = rxBuffer[0];
				_adg_set_flag(ADG2128_FLAG_INITIALIZED);
				ret = 0;
			}
			else {
				return -2;
			}
		}
	}
	return ret;
}


/*
* Returns a bitmask of all the columns connected to this row. This is the native
*   data returned from the part, so this lookup is cheap.
*/
//uint8_t getCols(uint8_t row) {
//  if (row > 11) return 0;
//  return _values[row];
//}


/*
* Returns a bitmask of all the rows connected to this column.
*/
//uint16_t getRows(uint8_t col) {
//  if (col > 7) return 0;
//  uint16_t ret = 0;
//  for (uint8_t i = 0; i < 12; i++) {
//    uint8_t val = (_values[i] >> col) & 1;
//    ret |= (val << i);
//  }
//  return ret;
//}


/*
* Setup the low-level pin details.
*/
static int8_t _ll_pin_init() {
  if (255 != _RESET_PIN) {
    GPIO_InitTypeDef  gpioinitstruct = {0};

    /* Enable the GPIO_LED clock */
    RST12_GPIO_CLK_ENABLE();

    /* Configure the GPIO_LED pin */
    gpioinitstruct.Pin    = RST1_PIN | RST2_PIN;
    gpioinitstruct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpioinitstruct.Pull   = GPIO_NOPULL;
    gpioinitstruct.Speed  = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(RST12_GPIO_PORT, &gpioinitstruct);

    HAL_GPIO_WritePin(RST12_GPIO_PORT, RST1_PIN, GPIO_PIN_SET); // Start part 1 in reset state.
    HAL_GPIO_WritePin(RST12_GPIO_PORT, RST2_PIN, GPIO_PIN_SET); // Start part 2 in reset state.
  }
  _adg_set_flag(ADG2128_FLAG_PINS_CONFD);
  return 0;
}


static int8_t _write_device(uint8_t row, uint8_t conn) {
	int8_t ret = -6;
	if (NULL != _bus) {
		uint8_t buffer[] = {row, conn};
		ret = (uint8_t)(I2C_Write(_ADDR, buffer, COUNTOF(buffer)));
	}
	return ret;
}

static void delay_adg(uint8_t timeout) {
	for (int i = 0; i < timeout; i++) {
		for (int i = 0; i < 4800; i++) {
		}
	}
}


