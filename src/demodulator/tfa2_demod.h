#ifndef _INCLUDE_TFA2_DEMODULATOR_H
#define _INCLUDE_TFA2_DEMODULATOR_H

// Bases Classes
#include "demodulator/demodulator.h"

class iir2;

class tfa2_demod: public demodulator
{
public:
   tfa2_demod(decoder *_dec, double spb, double iir_fac = 0.5);
   virtual ~tfa2_demod();

   int demod(int thresh, int pwr, int index, int16_t *iq);
   void reset();

private:
   double spb;
   int bitcnt;
   int dmin, dmax;
   int offset;
   int timeout_cnt;
   int last_i, last_q;
   int last_bit;
   int rssi;
   iir2 *iir;
   double est_spb;
};
#endif
