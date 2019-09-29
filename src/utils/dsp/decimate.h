/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _UTILS_DSP_DECIMATE_H
#define _UTILS_DSP_DECIMATE_H

#include <cstdint>

class decimate
{
public:
   decimate(void);
   virtual ~decimate(void);
   int process2x(int16_t *data, int length, int type); // IQ
   int process2x1(int16_t *data, int length); // IQ
private:
   int16_t hist[128];
   int16_t hist0[128];
   int16_t hist1[128];
};

#endif
