#include <math.h>
#include <string>

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
   last_bit_idx = 0;
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
         if (last_bit_idx)
         {
            // Determine number of 1-bits depending on time between 0-pulses
            if (index - last_bit_idx > 4)
            {
               for (int n = (2 * BITPERIOD) + 2; n <= (index - last_bit_idx); n += 2 * BITPERIOD)
               {
                  dec->store_bit(1);
               }
               dec->store_bit(0);
            }
         }
         if (index - last_bit_idx > 2)
         {
            last_bit_idx = index;
         }
      }
      // Flush data
      if (!timeout_cnt)
      {
         dec->flush(10 * log10(rssi));
         mark_lvl = 0;
         rssi = 0;
         last_bit_idx = 0;
      }
   }
   last_i = iq[0];
   last_q = iq[1];

   return triggered;
}
//-------------------------------------------------------------------------
