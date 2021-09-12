/*
 * queue.h
 *
 *  Created on: 13 ���. 2020 �.
 *      Author: Ivlichev_EP
 */

#ifndef INC_QUEUE_H_
#define INC_QUEUE_H_

#include "Control.h"

typedef struct _QUEUE_Itf
{
	CONTROL_ENM cmd;
	uint16_t buffer;
	uint8_t crc;
}QUEUE_TypeDef;

void qextract(void);
void qstore(uint8_t* Buf, uint32_t *Len);
void init_queue(void);

int CmdDataSelect(QUEUE_TypeDef *q);

#endif /* INC_QUEUE_H_ */
