#ifndef _INCLUDE_TFA5_DEMODULATOR_H
#define _INCLUDE_TFA5_DEMODULATOR_H

// Base Classes
#include "demodulator/demodulator.h"

// Helper
#include "utils/dsp_stuff.h"
#include "utils/crc32.h"

class whb_demod: public demodulator
{
public:
   whb_demod(decoder *_dec, double spb);
   virtual ~whb_demod();

   int demod(int thresh, int pwr, int index, int16_t *iq);
   void reset(void);

private:
   double spb;
   int bitcnt;
   int offset;
   int timeout_cnt;
   int last_i, last_q;
   int last_dev;
   uint64_t step;
   uint64_t last_peak;
   double rssi;
   iir2 *iir;
   iir2 *iir_avg;
   int avg_of;
};
#endif
