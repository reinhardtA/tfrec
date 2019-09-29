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

#include "downconvert.h"

downconvert::downconvert(int const &paramNumberPasses)
{
   passes = paramNumberPasses;
   dec_i.resize(passes);
   dec_q.resize(passes);
}
//-------------------------------------------------------------------------
downconvert::~downconvert(void)
{
}
//-------------------------------------------------------------------------
int downconvert::process_iq(int16_t *data_iq, int len, int filter_type)
{
   int ft = filter_type;
#if 1
   for (int i = 0; i < passes - 1; i++)
   {
      dec_i[i].process2x1(data_iq, len);
      dec_q[i].process2x1(data_iq + 1, len);

      len = len >> 1;
   }
#else
   for (int i=0; i < passes-1; i++) {
      dec_i[i].process2x(data_iq,   len);
      dec_q[i].process2x(data_iq+1, len);
      len=len>>1;
   }
#endif
   dec_i[passes - 1].process2x(data_iq, len, ft);
   dec_q[passes - 1].process2x(data_iq + 1, len, ft);

   len = len >> 1;

   return len;
}
