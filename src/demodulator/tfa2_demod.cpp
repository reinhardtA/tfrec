#include <math.h>

#include "demodulator/tfa2_demod.h"

//-------------------------------------------------------------------------
tfa2_demod::tfa2_demod(decoder *_dec, double _spb, double _iir_fac)
   :
   demodulator(_dec)
{
   spb = _spb;
   timeout_cnt = 0;
   reset();
   iir = new iir2(_iir_fac / spb); // iir_fac=0.5 -> Lowpass at bit frequency (01-pattern)
   printf("type 0x%x: Samples per bit: %.1f\n", _dec->get_type(), spb);
}
tfa2_demod::~tfa2_demod()
{
}
//-------------------------------------------------------------------------
void tfa2_demod::reset(void)
{
   offset = 0;
   bitcnt = 0;
   dmin = 32767;
   dmax = -32767;
   last_bit = 0;
   rssi = 0;
   est_spb = spb;
}
//-------------------------------------------------------------------------
//#define DBG_DUMP
#ifdef DBG_DUMP
// More debugging
FILE *fx=NULL;
FILE *fy=NULL;
int fc=0;
#endif

// Real FM/NRZ demodulation (compared to tfa1.cpp...)
// Note: index increases by 2 for each IQ-sample!
int tfa2_demod::demod(int thresh, int pwr, int index, int16_t *iq)
{
   int triggered = 0;
   int ld = 0;

   if (pwr > thresh)
   {
      if (!timeout_cnt)
         reset();

      timeout_cnt = 16 * spb;
   }

   if (timeout_cnt)
   {
      triggered++;

      int dev = fm_dev(iq[0], iq[1], last_i, last_q);
      ld = iir->step(dev);

      // Find deviation limits during sync word
      if (bitcnt < 10)
      {
         if (ld > dmax)
            dmax = (7 * dmax + ld) / 8;
         if (ld < dmin)
            dmin = (7 * dmin + ld) / 8;
         offset = (dmax + dmin) / 2;
         // Estimate power
         if (bitcnt > 4)
            rssi += (rssi + iq[0] * iq[0] + iq[1] * iq[1]) / 100;
      }
      timeout_cnt--;

      dev = ld;

      // cheap compensation of 0/1 asymmetry if deviation limited in preceeding filter
      int noffset = 0.9 * offset;

      int bit = 0;
      int margin = 32;

      // Hard decision
      if (dev > noffset + (dmax / margin))
         bit = 1;

      int bdbg = 0;
      if ((dev > noffset + dmax / margin || dev < noffset + dmin / margin) && bit != last_bit)
      {
         if (index > (last_bit_idx + 8))
         { // Ignore glitches
            bitcnt++;
            int tdiff = index - last_bit_idx;
            // Determine number of bits depending on time between edges
            if (tdiff > spb / 4 && tdiff < 32 * spb)
            {
               //printf("%i %i %i \n",bit,last_bit,(index-last_bit_idx)/2);
               int bit_diff = (index - last_bit_idx) / 2;
               int numbits = (bit_diff + (est_spb / 2)) / est_spb;
               if (numbits < 32)
               { // Sanity
                 //printf("   %i %i %i %i nb %i %.1f\n",  bit,last_bit,dev,(index-last_bit_idx)/2,numbits,fnumbits);
                  for (int n = 1; n < numbits; n++)
                  {
                     dec->store_bit(last_bit);
                  }
               }
               dec->store_bit(bit);
               last_bit = bit;
               bdbg = bit;
            }
         }
         if (index - last_bit_idx > 2)
            last_bit_idx = index;
      }

#ifdef DBG_DUMP
		if (!fx)
			fx=fopen("blub","w");
		if (!fy)
			fy=fopen("blub1","w");
		if (fx)
			fprintf(fx,"%i %i %i\n",fc,ld,bdbg*(noffset+dmin/margin));
		if (fy)
			fprintf(fy,"%i %i\n",fc,(bit?dmax:dmin));

		fc++;
#endif

      // Flush data
      if (!timeout_cnt)
      {
         // Add some trailing bits
         for (int n = 0; n < 16; n++)
            dec->store_bit(last_bit);

         //printf("MIN %i MAX %i OFFSET %i RSSI-Raw %i\n",dmin,dmax,offset,rssi);
         dec->flush(10 * log10(rssi), offset);
         reset();
      }
   }
   last_i = iq[0];
   last_q = iq[1];

   return triggered;
}
//-------------------------------------------------------------------------
