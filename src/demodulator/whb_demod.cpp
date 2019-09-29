#include <math.h>
#include <map>

#include "decoder/decoder.h"
#include "demodulator/whb_demod.h"

whb_demod::whb_demod(decoder *_dec, double _spb)
   :
   demodulator(_dec)
{
   spb = _spb;
   timeout_cnt = 0;
   reset();
   iir = new iir2(2.0 / spb); // Pulse filter
   iir_avg = new iir2(0.0025 / spb); // Phase discriminator filter
   printf("WHB: Samples per bit: %.1f\n", spb);
   last_dev = 0;
}
whb_demod::~whb_demod()
{
}
//-------------------------------------------------------------------------
void whb_demod::reset(void)
{
   offset = 0;
   bitcnt = 0;
   last_peak = 0;
   rssi = 0;
   step = last_peak = 0;
}
//-------------------------------------------------------------------------
//#define DBG_DUMP
#if DBG_DUMP
// More debugging
static FILE *fx=NULL;
static FILE *fy=NULL;
static int fc=0;
#endif
int whb_demod::demod(int thresh, int pwr, int index, int16_t *iq)
{
   int triggered = 0;

   if (pwr > thresh)
   {
      if (!timeout_cnt)
      {
         reset();
      }

      timeout_cnt = 8 * spb;
   }

   if (timeout_cnt)
   {
      triggered++;
      int dev;

      /* Shaped PSK of AX5031 causes hard drop at phase changes for fm_dev_nrzs()
       -> detected minima are 0s, fillup with 1s since last 0
       */
      dev = fm_dev_nrzs(iq[0], iq[1], last_i, last_q);
      dev = iir->step(dev); // reduce noise
      if (!m_ptrDecoder->has_sync())
      {
         avg_of = iir_avg->step(0.5 * dev); // decision value for phase change
      }

      int bit = 0;
      timeout_cnt--;

      int tdiff = step - last_peak;

      // Phase change?
      if (dev < avg_of &&
         dev > last_dev &&
         (tdiff > 3 * spb / 4))
      {
         bit = avg_of;
         m_ptrDecoder->store_bit(0);
         bitcnt++;
         int bit0 = (tdiff + spb / 2) / spb;
         for (int n = 1; n < bit0; n++)
         {
            m_ptrDecoder->store_bit(1);
            bitcnt++;
         }
         last_peak = step;
      }
      last_dev = dev;

      if (m_ptrDecoder->has_sync())
      {
         rssi += (iq[0] * iq[0] + iq[1] * iq[1]);
      }

#ifdef DBG_DUMP
		//  plot "blub" using 1:2 with lines,"blub" using 1:3 with boxes
		if (!fx)
			fx=fopen("blub","w");
		if (!fy)
			fy=fopen("blub1","w");
		if (fx)
			fprintf(fx,"%i %i %i %i\n",fc,dev, bit, tdiff_mod*10);
		fc++;
#endif

      if (!timeout_cnt)
      {
         // Flush descrambler
         if (m_ptrDecoder->has_sync())
         {
            for (int n = 0; n < 16; n++)
            {
               m_ptrDecoder->store_bit(0);
            }
            m_ptrDecoder->flush(10 * log10(1 + rssi / 4000), offset); // scale to rougly match with TFA_1-RSSI
         }
         reset();
         rssi = 0;
      }
   }
   last_i = iq[0];
   last_q = iq[1];

   step++;
   return triggered;
}
//-------------------------------------------------------------------------
