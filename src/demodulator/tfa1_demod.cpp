#include <math.h>
#include <string>

#include "decoder/decoder.h"
#include "demodulator/tfa1_demod.h"
#include "utils/dsp_stuff.h"

// real samplerate 1536kHz, after 4x-decimation 384kHz
#define BITPERIOD ((1536000/38400)/4)

//-------------------------------------------------------------------------
tfa1_demod::tfa1_demod(decoder *_dec)
   :
   demodulator(_dec)
{
   timeout_cnt = 0;
   reset();
}
tfa1_demod::~tfa1_demod()
{
}
void tfa1_demod::reset()
{
   //TODO: 20190927 : added this lines - if this leads to trouble remove them
   mark_lvl = 0;
   rssi = 0;
   m_IdxLastBit = 0;
   last_i = 0;
   last_q = 0;
}
//-------------------------------------------------------------------------
int tfa1_demod::demod(int thresh, int pwr, int index, int16_t *iq)
{
   int triggered = 0;

   if (pwr > thresh)
   {
      timeout_cnt = 40 * BITPERIOD;
   }

   if (timeout_cnt)
   {
      triggered++;
      int dev = fm_dev_nrzs(iq[0], iq[1], last_i, last_q);

      // Hold maximum deviation of 0-edges for reference
      if (dev > mark_lvl)
      {
         mark_lvl = dev;
      }
      else
      {
         mark_lvl = mark_lvl * 0.95;
      }

      // remember peak for RSSI look-alike
      if (mark_lvl > rssi)
      {
         rssi = mark_lvl;
      }

      timeout_cnt--;
      // '0'-pulse if deviation drops below referenced threshold
      if (dev < mark_lvl / 2)
      {
         if (0 != m_IdxLastBit)
         {
            // Determine number of 1-bits depending on time between 0-pulses
            if (index - m_IdxLastBit > 4)
            {
               for (int n = (2 * BITPERIOD) + 2; n <= (index - m_IdxLastBit); n += 2 * BITPERIOD)
               {
                  m_ptrDecoder->store_bit(1);
               }
               m_ptrDecoder->store_bit(0);
            }
         }
         if ((index - m_IdxLastBit) > 2)
         {
            m_IdxLastBit = index;
         }
      }
      // Flush data
      if (!timeout_cnt)
      {
         m_ptrDecoder->flush(10 * log10(rssi));
         mark_lvl = 0;
         rssi = 0;
         m_IdxLastBit = 0;
      }
   }
   last_i = iq[0];
   last_q = iq[1];

   return triggered;
}
//-------------------------------------------------------------------------
