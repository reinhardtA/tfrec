#ifndef _INCLUDE_DEMODULATOR_H
#define _INCLUDE_DEMODULATOR_H

#include "decoder/decoder.h"

class demodulator
{
public:
   demodulator(decoder *_dec);
   virtual ~demodulator();

   virtual void start(int len);
   virtual void reset();
   virtual int demod(int thresh, int pwr, int index, int16_t *iq);

   // TODO : make me private
   decoder *dec;

protected:

   int last_bit_idx;

};

#endif
