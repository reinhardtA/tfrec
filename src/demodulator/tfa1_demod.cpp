#include <math.h>
#include <string>

#include "decoder/decoder.h"
#include "demodulator/tfa1_demod.h"

#include "utils/defines.h"
#include "utils/dsp_stuff.h"

//-------------------------------------------------------------------------
tfa1_demod::tfa1_demod(decoder *_dec)
   :
   demodulator(_dec)
{
   m_Timeout_Counter = 0;
   reset();
}
tfa1_demod::~tfa1_demod()
{
}
void tfa1_demod::reset()
{
   //TODO: 20190927 : added this lines - if this leads to trouble remove them
   m_Mark_Level = 0;
   m_RSSI = 0;
   m_IdxLastBit = 0;
   m_last_i = 0;
   m_last_q = 0;
}
//-------------------------------------------------------------------------
int tfa1_demod::demod(int thresh, int pwr, int index, int16_t *iq)
{
   int tTriggered = 0;

   // check signal power against threashold
   if (pwr > thresh)
   {
      m_Timeout_Counter = 40 * BITPERIOD;
   }

   if (0 != m_Timeout_Counter)
   {
      tTriggered++;
      int tDeviation = fm_dev_nrzs(iq[0], iq[1], m_last_i, m_last_q);

      // Hold maximum deviation of 0-edges for reference
      if (tDeviation > m_Mark_Level)
      {
         m_Mark_Level = tDeviation;
      }
      else
      {
         // otherwise decrease the level by 5%
         m_Mark_Level = m_Mark_Level * 0.95;
      }

      // remember peak for RSSI look-alike
      if (m_Mark_Level > m_RSSI)
      {
         m_RSSI = m_Mark_Level;
      }

      m_Timeout_Counter--;
      // '0'-pulse if deviation drops below referenced threshold
      if (tDeviation < m_Mark_Level / 2)
      {
         if (0 != m_IdxLastBit)
         {
            // Determine number of 1-bits depending on time between 0-pulses
            if ((index - m_IdxLastBit) > 4)
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
      if (0 == m_Timeout_Counter)
      {
         m_ptrDecoder->flush(10 * log10(m_RSSI), 0);
         m_Mark_Level = 0;
         m_RSSI = 0;
         m_IdxLastBit = 0;
      }
   }
   m_last_i = iq[0];
   m_last_q = iq[1];

   return tTriggered;
}
//-------------------------------------------------------------------------
