/*
 * queue.c
 *
 *  Created by: Neurosoft Co.
 *      Author: Ivlichev_EP
 */
#include "main.h"
#include "queue.h"

#define QUEUE_MAX 15
static QUEUE_TypeDef p[QUEUE_MAX+1];
static QUEUE_TypeDef GET_NULL = {.cmd = 0xFF, .buffer = 0x0000, .crc = 0xFF};

static int spos = 0;
static int rpos = 0;

void init_queue()
{
	spos = 0;
	rpos = 0;
}

/* Store event. */
void qstore(uint8_t* Buf, uint32_t *Len)
{
	QUEUE_TypeDef q;

	if(spos+1==rpos || (spos+1==QUEUE_MAX && !rpos)) {
		return;
	}

	if (Len[0] != 4) {
		return;
	}

	q.cmd = (CONTROL_ENM)Buf[0];
	q.buffer = (uint16_t)Buf[1] | ((uint16_t)Buf[2] << 8);
	q.crc = Buf[3];

	p[spos] = q;
	spos++;
	if(spos==QUEUE_MAX) spos = 0;
}

/* Retrieve event. */
static QUEUE_TypeDef *qretrieve()
{
  if(rpos==QUEUE_MAX) rpos = 0;
  if(rpos==spos) {
    return (QUEUE_TypeDef *)&GET_NULL;
  }
  rpos++;
  return &p[rpos-1];
}

/* Extract event. */
void qextract()
{
	QUEUE_TypeDef *dataIn = qretrieve();

	if (dataIn->cmd != 0xFF)
	{
		CmdDataSelect(dataIn);
	}
}



