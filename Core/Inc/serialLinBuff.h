#include "stm32f0xx_hal.h"

#ifndef SERIAL_LIN_BUFF_H
#define SERIAL_LIN_BUFF_H

#define LIN_BUFF_SIZE 64

typedef struct {
    uint8_t currIndex;
    char buff[LIN_BUFF_SIZE];
} serialLinBuff_t;

void serialLinBuffReset(serialLinBuff_t *serialLinBuff);
void serialLinBuffAddChar(serialLinBuff_t *serialLinBuff, char c);
uint8_t serialLinBuffReady(serialLinBuff_t *serialLinBuff);

#endif // SERIAL_LIN_BUFF_H
