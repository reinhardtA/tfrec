/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <gplv2.h>
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "fm_demod.h"
#include "demodulator/demodulator.h"
#include "utils/dsp_stuff.h"

//-------------------------------------------------------------------------
fsk_demod::fsk_demod(std::vector<demodulator*> *_demods, int _thresh, int _dbg)
{
   m_Logger = "FSK-DEMOD";
   m_DebugLevel = _dbg;
   m_ptrDemodulators = _demods;
   m_Threshold = _thresh;
   m_Tthreshold_Mode = eThresholMode::eUser;

   if (m_Threshold == 0)
   {
      m_Threshold = 500; // default
      m_Tthreshold_Mode = eThresholMode::eAuto;
   }
   m_Last_i = 0;
   m_Last_q = 0;

   m_ProcessTriggered_Avg = 0;
   m_NumberRuns = 0;
}
fsk_demod::~fsk_demod()
{
}
//-------------------------------------------------------------------------
void fsk_demod::process(int16_t *data_iq, int paramLength)
{
   int tProcessTriggered = 0;
   m_NumberRuns++;

   // TODO : think about iterators ...
   for (size_t n = 0; n < m_ptrDemodulators->size(); n++)
   {
      m_ptrDemodulators->at(n)->start(paramLength);
   }

   // decode data of length paramLength, +2 due to i/q data
   for (int idxIQdata = 0; idxIQdata < paramLength; idxIQdata += 2)
   {
      // Trigger decoding at power level and check some bit periods
      int tSignalPower = abs(data_iq[idxIQdata]) + abs(data_iq[idxIQdata + 1]);
      int tDemodTrigger = 0;

      // demodulate with every known demodulator
      // TODO : think about iterators
      for (std::size_t n = 0; n < m_ptrDemodulators->size(); n++)
      {
         tDemodTrigger += m_ptrDemodulators->at(n)->demod(m_Threshold, tSignalPower, idxIQdata, data_iq + idxIQdata);
      }

      if (0 != tDemodTrigger)
      {
         tProcessTriggered++;
      }

      // save the current symbol for deviation calculation
      m_Last_i = data_iq[idxIQdata];
      m_Last_q = data_iq[idxIQdata + 1];
   }

   // calc kind of weighted average
   m_ProcessTriggered_Avg = (31 * m_ProcessTriggered_Avg + tProcessTriggered) / 32;

   if (m_DebugLevel)
   {
      // paramLength / 2 -> cause of IQ Data
      printf("Trigger ratio %i/%i, avg %i \n", tProcessTriggered, paramLength / 2, m_ProcessTriggered_Avg);
   }

   if ((eThresholMode::eAuto == m_Tthreshold_Mode) && ((m_NumberRuns & 3) == 0))
   {
      if (m_ProcessTriggered_Avg >= paramLength / 32)
      {
         m_Threshold += 2;
         if (m_DebugLevel)
         {
            printf("Increased trigger level to %i\n", m_Threshold);
         }
      }
      else if ((m_ProcessTriggered_Avg <= paramLength / 64) && (m_Threshold > 50))
      {
         m_Threshold -= 2;
         if (m_DebugLevel)
         {
            printf("Decreased trigger level to %i\n", m_Threshold);
         }
      }
   }
}
//-------------------------------------------------------------------------
