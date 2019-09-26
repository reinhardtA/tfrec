#ifndef _INCLUDE_TFA1H
#define _INCLUDE_TFA1H

// Base Classes
#include "decoder.h"
#include "demodulator.h"

// Helper
#include "crc8.h"

class tfa1_decoder: public decoder
{
public:
   tfa1_decoder(sensor_e _type);
   virtual ~tfa1_decoder()
   {
   }
   void store_bit(int bit);

   // TODO : remove default value
   void flush(int rssi, int offset = 0);

private:
   uint32_t sr;
   int sr_cnt;
   int snum;
   crc8 *crc;
};

class tfa1_demod: public demodulator
{
public:
   tfa1_demod(decoder *_dec);
   virtual ~tfa1_demod()
   {
   }
   int demod(int thresh, int pwr, int index, int16_t *iq);

private:
   int timeout_cnt;
   int last_i, last_q;
   int mark_lvl;
   int rssi;
};
#endif
