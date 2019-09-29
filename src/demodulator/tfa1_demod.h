#ifndef _INCLUDE_TFA1DEMOD_H
#define _INCLUDE_TFA1DEMOD_H

// Base Classes
#include "demodulator/demodulator.h"

class tfa1_demod: public demodulator
{
public:
   tfa1_demod(decoder *_dec);
   virtual ~tfa1_demod();

   int demod(int thresh, int pwr, int index, int16_t *iq);
   void reset();

private:
   int m_last_i;
   int m_last_q;

   int timeout_cnt;

   int mark_lvl;
   int rssi;
};
#endif
