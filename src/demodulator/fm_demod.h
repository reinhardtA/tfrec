/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _INCLUDE_FM_DEMOD_H
#define _INCLUDE_FM_DEMOD_H

#include <vector>
#include <cstdint>

// as long as we use just ptr, we may forward decl. this class
class demodulator;

class fsk_demod
{
public:
   fsk_demod(std::vector<demodulator*> *_demods, int _thresh, int _dbg);
   virtual ~fsk_demod();

   void process(int16_t *data_iq, int len);

private:
   int thresh;
   int thresh_mode;
   int triggered_avg;
   int runs;
   int dbg;

   int16_t last_i;
   int16_t last_q;
   uint64_t index;

   std::vector<demodulator*> *demods;

};
#endif
