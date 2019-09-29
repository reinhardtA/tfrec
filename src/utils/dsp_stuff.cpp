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

#include <math.h>
#include "dsp_stuff.h"

//-------------------------------------------------------------------------
// Vastly reduced mixed demodulator for FM-NRZS:
// It doesn't need to be linear, nor do we care about frequency shift direction
//-------------------------------------------------------------------------
int fm_dev_nrzs(int ar, int aj, int br, int bj)
{
   int cr = ar * br + aj * bj;

   // This limits also the max RSSI
   if (cr > 1e9)
   {
      cr = 1e9;
   }
   if (cr < -1e9)
   {
      cr = -1e9;
   }
   return cr;
}
//-------------------------------------------------------------------------
// Real FM demodulation
//-------------------------------------------------------------------------
#if 1
int fm_dev(int ar, int aj, int br, int bj)
{
   double cr, cj;
   double angle;
   cr = ((double) ar) * br + ((double) aj) * bj;
   cj = ((double) aj) * br - ((double) ar) * bj;
   angle = atan2(cj, cr);
   return (int) (angle / M_PI * (1 << 14));
}
#endif
//-------------------------------------------------------------------------
