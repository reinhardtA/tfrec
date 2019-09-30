/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#include "engine.h"

#include "utils/defines.h"
#include "utils/dsp/downconvert.h"

#define RLS 65536

//-------------------------------------------------------------------------
engine::engine(int _device, uint32_t paramFrequency, int gain, int filter, fsk_demod *_fsk, int _dbg, int _dumpmode, char *_dumpfile)
{
   m_Logger = "ENG";
   m_SampleRate = 1536000;
   //m_SampleRate = 768000; // does also work with TFA1 ..
   m_Frequency = paramFrequency * 1000; // kHz->Hz

   filter_type = filter;
   fsk = _fsk;

   dbg = _dbg;
   dumpfile = _dumpfile;
   dumpmode = _dumpmode;

   if (0 != filter_type)
   {
      puts("Wide filter");
   }

   if (0 != dumpmode)
   {
      printf("Dumpmode %i (%s), dumpfile %s\n", dumpmode, dumpmode == 1 ? "SAVE" : (dumpmode == -1 ? "LOAD" : "NONE"), dumpfile);
   }

   if (dumpmode >= 0)
   {
      m_ptrSdr = new sdr(_device, dbg, dumpmode, dumpfile);
      if (NULL != m_ptrSdr)
      {
         if (gain == -1)
         {
            m_ptrSdr->set_gain(eGainMode::eAuto, 0);
         }
         else
         {
            m_ptrSdr->set_gain(eGainMode::eUser, gain);
         }
         m_ptrSdr->set_frequency(m_Frequency);
         m_ptrSdr->set_samplerate(m_SampleRate);
      }
      else
      {
         printf("(%s) %s : %s\n", "ERR", m_Logger.c_str(), "cant init SDR device!");
         exit(-1);
      }
   }
}
//-------------------------------------------------------------------------
engine::~engine()
{
   if (NULL != m_ptrSdr)
   {
      delete m_ptrSdr;
   }
}
//-------------------------------------------------------------------------
// timeout = 0 -> forever
// dumpmode = -1 -> LOAD Data from File
// dumpmode = 0 | 1 -> SDR Only | SAVE Data
void engine::run(int timeout)
{
   FILE *tptrDumpData = NULL;

   if (dumpmode >= 0)
   {
      m_ptrSdr->start();
   }
   else
   {
      tptrDumpData = fopen(dumpfile, "rb");
      if (NULL == tptrDumpData)
      {
         printf("(%s) %s : %s\n", "ERR", m_Logger.c_str(), "cant open dump file!");
         perror(dumpfile);
         exit(-1);
      }
   }
   downconvert tDonwConverter(2);

   time_t start = time(0);

   // endless loop
   while (true)
   {
      int16_t *data;
      int len;

      if (NULL != tptrDumpData)
      {
         int16_t datab[RLS];
         {
            unsigned char buf[RLS];
            int xx = fread(buf, RLS, 1, tptrDumpData);
            if (xx < 1)
            {
               printf("done reading dump\n");
               exit(0);
            }
            for (int n = 0; n < RLS; n++)
            {
               datab[n] = ((buf[n]) - 128) << 6; // scale like in sdr.cpp
            }
            data = datab;
            len = RLS;
         }
      }
      else
      {
         // wait for new data
         m_ptrSdr->wait(data, len); // len=total sample number = #i+#q
      }

      //printf("origin : %i\n", len);
      int ld = tDonwConverter.process_iq(data, len, filter_type);
      //printf("down   : %i\n", ld);
      fsk->process(data, ld);

      if (NULL != tptrDumpData)
      {
         ;
      }
      else
      {
         m_ptrSdr->done(len);
      }

      // check for timeout hit
      if ((0 != timeout) && ((time(0) - start) > timeout))
      {
         break;
      }
   }
}
//-------------------------------------------------------------------------
