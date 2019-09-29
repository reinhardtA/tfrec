/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _UTILS_DSP_IIR2_H
#define _UTILS_DSP_IIR2_H

// infinite impulse response filter ...
class iir2
{
public:
   iir2(double cutoff);
   virtual ~iir2();

   double step(double din);
   void set(double cutoff);

private:
   double dn1, dn2;
   double yn, yn1, yn2;
   double b0, b1, b2, a1, a2;
};

#endif
