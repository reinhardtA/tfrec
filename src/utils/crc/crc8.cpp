#include "crc8.h"

//-------------------------------------------------------------------------
crc8::crc8(int paramPolynom)
{
   int n;
   int m;
   for (n = 0; n < 256; n++)
   {
      int t = n;
      for (m = 0; m < 8; m++)
      {
         if ((t & 0x80) != 0)
         {
            t = (t << 1) ^ paramPolynom;
         }
         else
         {
            t = t << 1;
         }
      }
      m_Lookup[n] = t;
   }
}
crc8::~crc8()
{
}
//-------------------------------------------------------------------------
uint8_t crc8::calc(uint8_t const *data, int len)
{
   int n;
   uint8_t tRetCRC = 0;

   for (n = 0; n < len; n++)
   {
      tRetCRC = m_Lookup[tRetCRC ^ *data];
      data++;
   }
   return tRetCRC;
}
