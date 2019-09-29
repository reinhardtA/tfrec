#ifndef _INCLUDE_TFA1H
#define _INCLUDE_TFA1H

// Base Classes
#include "decoder/decoder.h"

// Helper
#include "utils/crc/crc8.h"

class tfa1_decoder: public decoder
{
public:
   tfa1_decoder(sensor_e _type);
   virtual ~tfa1_decoder();

   /**
    * this function stores the information, if applicable
    */
   void store_bit(int bit);

   // TODO : remove default value
   void flush(int rssi, int offset);

private:
   uint32_t sr;
   int sr_cnt;
   int snum;

   // a (simple) CRC8 Calculator
   crc8 *m_ptrCrcPolynom;
};

#endif
