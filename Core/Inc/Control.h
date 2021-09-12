/*
 * Control.h
 *
 *  Created on: Sep 11, 2021
 *      Author: evgenijivlicev
 */

#ifndef INC_CONTROL_H_
#define INC_CONTROL_H_

typedef enum {
  USB_POWER_EN  = 0,
  USB_DETECT    = 1,
  BREATH_MODE   = 2,
  BREATH_PERIOD = 3,
  BREATH_GEN1   = 4,
  BREATH_GEN2   = 5,
  MUX_AD1       = 6,
  MUX_AD2       = 7
} CONTROL_ENM;

void CmdInit(void);
void BreathTimer(void);

#endif /* INC_CONTROL_H_ */
