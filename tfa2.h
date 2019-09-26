#ifndef _INCLUDE_TFA2H
#define _INCLUDE_TFA2H

#include <string>

// BAses Classes
#include "decoder.h"
#include "demodulator.h"

// Helper
#include "dsp_stuff.h"
#include "crc8.h"

// TODO : remove
using std::string;

class tfa2_decoder: public decoder
{
public:
   tfa2_decoder(sensor_e type = TFA_2);
   void store_bit(int bit);
   void flush(int rssi, int offset = 0);
   private:
   void flush_tfa(int rssi, int offset = 0);
   void flush_tx22(int rssi, int offset = 0);
   int invert;
   uint32_t sr;
   int sr_cnt;
   int snum;
   crc8 *crc;
};

class tfa2_demod: public demodulator
{
public:
   tfa2_demod(decoder *_dec, double spb, double iir_fac = 0.5);
   void reset(void);
   int demod(int thresh, int pwr, int index, int16_t *iq);

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
