/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _INCLUDE_ENGINE_H
#define _INCLUDE_ENGINE_H

#include <unistd.h>
#include <stdlib.h>
#include <string>

#include "sdr.h"
#include "utils/dsp_stuff.h"
#include "demodulator/fm_demod.h"

class engine
{
public:
   engine(int device, uint32_t freq, int gain, int filter, fsk_demod *fsk, int dbg, int dmpmode, char *dumpfile);
   ~engine();
   void run(int timeout);

private:
   // the logger
   std::string m_Logger;

   fsk_demod *fsk;

   // the sample rate
   int m_SampleRate;
   // frequency in Hz
   uint32_t m_Frequency;

   int filter_type;
   int dbg;

   sdr *m_ptrSdr;
   int dumpmode;
   char *dumpfile;
};

#endif
