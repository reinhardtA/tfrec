#include "demodulator.h"

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
demodulator::demodulator(decoder *_dec)
   :
   m_ptrDecoder(_dec)
      , m_IdxLastBit(0)
{
}

demodulator::~demodulator()
{
}

//-------------------------------------------------------------------------
void demodulator::start(int len)
{
   if (m_IdxLastBit)
   {
      // handle data wrap from last block processing
      m_IdxLastBit -= len;
   }
}
