#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "demodulator.h"

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
demodulator::demodulator(decoder *_dec)
{
   dec = _dec;
   last_bit_idx = 0;
}
//-------------------------------------------------------------------------
void demodulator::start(int len)
{
   if (last_bit_idx)
   {
      // handle data wrap from last block processing
      last_bit_idx -= len;
   }

}
//-------------------------------------------------------------------------
int demodulator::demod(int thresh, int pwr, int index, int16_t *iq)
{
   return 0;
}
//-------------------------------------------------------------------------
void demodulator::reset(void)
{

}
