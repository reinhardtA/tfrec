/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _UTILS_DSP_DOWNCONVERT_H
#define _UTILS_DSP_DOWNCONVERT_H

#include <vector>

#include "utils/dsp/decimate.h"

class downconvert
{
public:
   downconvert(int const &paramNumberPasses);
   virtual ~downconvert();
   int process_iq(int16_t *buf, int len, int filter = 0);

private:
   std::vector<decimate> dec_i;
   std::vector<decimate> dec_q;
   int passes;
};

#endif
