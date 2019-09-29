/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>

 dsp_stuff.cpp - Some useful DSP functions

 Some parts inspired from librtlsdr (rtl_fm.c)
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 * Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>


 */

#include "decimate.h"

//-------------------------------------------------------------------------
// In-place decimator
#define DEC_TAP_NUM1 20
#define DEC_TAP_NUM2 8

static int16_t dec_filter_taps1[DEC_TAP_NUM1] =
   {
      /* http://t-filter.engineerjs.com/
       20 taps, sampling frequency 768Hz, 16bit
       0..44Hz @ 1, Ripple 2dB
       96...384Hz@0, att -35dB
       */
      -1087,
      -1082,
      -1065,
      -451,
      912,
      2997,
      5556,
      8157,
      10285,
      11484,
      11484,
      10285,
      8157,
      5556,
      2997,
      912,
      -451,
      -1065,
      -1082,
      -1087
   };

// 90% wider filter
static int16_t dec_filter_taps1w[DEC_TAP_NUM1] =
   {
      /*
       0..80Hz @ 1, Ripple 2dB
       144...384Hz@0, att -35dB
       */
      546,
      451,
      -317,
      -1844,
      -3198,
      -2817,
      494,
      6469,
      13074,
      17421,
      17421,
      13074,
      6469,
      494,
      -2817,
      -3198,
      -1844,
      -317,
      451,
      546
   };

static int16_t dec_filter_taps2[DEC_TAP_NUM2] =
   {
      // 0..48@4, 160-384@-36, 8taps
      2443,
      6339,
      11036,
      14254,
      14254,
      11036,
      6339,
      2443
   };
// 30% wider filter
static int16_t dec_filter_taps2w[DEC_TAP_NUM2] =
   {
      // 0..64@4, 180-384@-36, 8taps
      2121,
      6697,
      12736,
      17117,
      17117,
      12736,
      6697,
      2121,
   };

decimate::decimate(void)
{
   for (int i = 0; i < 2 * DEC_TAP_NUM1; i++)
   {
      hist[i] = 0;
      hist0[i] = 0;
      hist1[i] = 0;
   }
}
//-------------------------------------------------------------------------
decimate::~decimate(void)
{
}

//-------------------------------------------------------------------------
/*
 0 2 4 6 8 0 2 4-6 8 0 2 4 6 8 0-2 4 6
 0   x x X X
 1       x x X X
 2           x x X X
 3               x x X X-
 4                   x x-X X
 5                      -x x X X
 6                           x x X X
 7                               x x X X-
 8                                   x x-X X
 */
// Data: IQ with int16_t (-> step 2 for each sample)
int decimate::process2x(int16_t *data, int length, int type)
{
   int shift = 16;
   int16_t t0[DEC_TAP_NUM1];
   int16_t *taps = dec_filter_taps1;
   if (type)
   {
      taps = dec_filter_taps1w;
   }

   for (int i = 0; i < DEC_TAP_NUM1; i++)
   {
      t0[i] = hist0[i];
   }
   int32_t sum;
   for (int i = 0; i < length; i += 4)
   {

      for (int n = 0; n < DEC_TAP_NUM1 - 2; n++)
         t0[n] = t0[n + 2];

      t0[DEC_TAP_NUM1 - 2] = data[i];
      t0[DEC_TAP_NUM1 - 1] = data[i + 2];

      sum = 0;

      for (int n = 0; n < DEC_TAP_NUM1; n++)
         sum += (t0[n] * taps[n]) >> shift;

      data[i / 2] = sum;
   }
   for (int i = 0; i < DEC_TAP_NUM1; i++)
      hist0[i] = t0[i];
   return 0;
}
//-------------------------------------------------------------------------
int decimate::process2x1(int16_t *data, int length)
{
   int shift = 16;
   int16_t t0[DEC_TAP_NUM2];
   int16_t *taps = dec_filter_taps2;
   for (int i = 0; i < DEC_TAP_NUM2; i++)
   {
      t0[i] = hist0[i];
   }

   int32_t sum;
   for (int i = 0; i < length; i += 4)
   {

      for (int n = 0; n < DEC_TAP_NUM2 - 2; n++)
      {
         t0[n] = t0[n + 2];
      }

      t0[DEC_TAP_NUM2 - 2] = data[i];
      t0[DEC_TAP_NUM2 - 1] = data[i + 2];
      sum = 0;

      for (int n = 0; n < DEC_TAP_NUM2; n++)
      {
         sum += (t0[n] * taps[n]) >> shift;
      }

      data[i / 2] = sum;
   }
   for (int i = 0; i < DEC_TAP_NUM2; i++)
   {
      hist0[i] = t0[i];
   }
   return 0;
}
