/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _INCLUDE_FM_DEMOD_H
#define _INCLUDE_FM_DEMOD_H

#include <string>
#include <vector>
#include <cstdint>

#include "utils/defines.h"

// as long as we use just ptr, we may forward decl. this class
class demodulator;

class fsk_demod
{
public:
   /**
    * keep in mind : paramThreashold = 0 -> Threashold modus will be "Auto"
    */
   fsk_demod(std::vector<demodulator*> *_demods, int _thresh, int _dbg);
   virtual ~fsk_demod();

   /**
    * process i/q Data with the length
    */
   void process(int16_t *data_iq, int paramLength);

protected:
   std::string m_Logger;

private:
   // Debug Modus
   int m_DebugLevel;
   // Modus for Threshold calculation
   eThresholMode m_Tthreshold_Mode;
   // Threshold for demodulation
   int m_Threshold;
   // weighted average of trigger
   int m_ProcessTriggered_Avg;
   // number of process runs (will be used for threadshold auto mode)
   int m_NumberRuns;

   // last i/q values for deviation calculation
   int16_t m_Last_i;
   int16_t m_Last_q;

   // known demodulator
   std::vector<demodulator*> *m_ptrDemodulators;

};
#endif
