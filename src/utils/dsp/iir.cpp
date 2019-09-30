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

#include "iir.h"

//-------------------------------------------------------------------------
// 2nd order butterworth lowpass
iir2::iir2(double const &paramCutOffFreqeuncy)
{
   yn = yn1 = yn2 = 0;
   dn1 = dn2 = 0;
   setCutOffFrequency(paramCutOffFreqeuncy);
}
iir2::~iir2()
{
}
//-------------------------------------------------------------------------
void iir2::setCutOffFrequency(double const &paramCutOffFreqeuncy)
{
   double i = 1.0 / tan(M_PI * paramCutOffFreqeuncy);

   //TODO : may be optimized as a "constant" somewhere
   //TODO : code to proove speedup as unittest
   double s = sqrt(2);
   b0 = 1 / (1 + s * i + i * i);
   b1 = 2 * b0;
   b2 = b0;
   a1 = 2 * (i * i - 1) * b0;
   a2 = -(1 - s * i + i * i) * b0;
}
//-------------------------------------------------------------------------
double iir2::step(double const &din)
{
   yn2 = yn1;
   yn1 = yn;
   yn = b0 * din + b1 * dn1 + b2 * dn2 + a1 * yn1 + a2 * yn2;
   dn2 = dn1;
   dn1 = din;
   return yn;
}
