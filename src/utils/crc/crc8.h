#ifndef _INCLUDE_CRC8_H
#define _INCLUDE_CRC8_H

#include <stdint.h>

class crc8
{
public:
   crc8(int poly);
   virtual ~crc8();

   /**
    * calculates a CRC from data with a given length
    */
   uint8_t calc(uint8_t const *data, int len);

private:
   uint8_t m_Lookup[256];

};

#endif

