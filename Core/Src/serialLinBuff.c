#include <string.h>
#include "serialLinBuff.h"

void serialLinBuffReset(serialLinBuff_t *serialLinBuff)
{
    serialLinBuff->currIndex = 0;
    memset(serialLinBuff->buff, 0, sizeof(serialLinBuff->buff));
}

void serialLinBuffAddChar(serialLinBuff_t *serialLinBuff, char c)
{
    if (serialLinBuff->currIndex < LIN_BUFF_SIZE - 1)
    {
        serialLinBuff->buff[serialLinBuff->currIndex] = c;
        serialLinBuff->currIndex++;
    }
}

uint8_t serialLinBuffReady(serialLinBuff_t *serialLinBuff)
{
    if(serialLinBuff->buff[serialLinBuff->currIndex - 1] == '\n')
    {
        return 1;
    }
    return 0;
}