#ifndef _INCLUDE_TFA2H
#define _INCLUDE_TFA2H

// Bases Classes
#include "decoder/decoder.h"

// Helper
#include "utils/crc/crc8.h"
#include "utils/dsp_stuff.h"

class tfa2_decoder: public decoder
{
public:
   tfa2_decoder(sensor_e type = TFA_2);
   virtual ~tfa2_decoder();
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

#endif
