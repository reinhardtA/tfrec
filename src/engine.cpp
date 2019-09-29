/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#include "engine.h"

#include "utils/dsp/downconvert.h"

#define RLS 65536

//-------------------------------------------------------------------------
engine::engine(int _device, uint32_t freq, int gain, int filter, fsk_demod *_fsk, int _dbg, int _dumpmode, char *_dumpfile)
{
   m_SampleRate = 1536000;

   freq = freq * 1000; // kHz->Hz
   filter_type = filter;
   fsk = _fsk;
   dbg = _dbg;
   dumpfile = _dumpfile;
   dumpmode = _dumpmode;

   if (filter_type)
   {
      puts("Wide filter");
   }

   if (dumpmode)
   {
      printf("Dumpmode %i (%s), dumpfile %s\n", dumpmode,
         dumpmode == 1 ? "SAVE" : (dumpmode == -1 ? "LOAD" : "NONE"),
         dumpfile);
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
         m_ptrSdr->set_frequency(freq);
         m_ptrSdr->set_samplerate(m_SampleRate);
      }
      else
      {
         //TODO: print error!
         exit(-1);
      }
   }
}
//-------------------------------------------------------------------------
engine::~engine(void)
{
}
//-------------------------------------------------------------------------
void engine::run(int timeout)
{
   FILE *dump_fd = NULL;

   if (dumpmode >= 0)
   {
      m_ptrSdr->start();
   }
   else
   {
      dump_fd = fopen(dumpfile, "rb");
      if (!dump_fd)
      {
         perror(dumpfile);
         exit(-1);
      }
   }
   downconvert dc(2);

   time_t start = time(0);

   while (true)
   {
      int16_t *data;
      int len;

      if (dump_fd)
      {
         int16_t datab[RLS];
         {
            unsigned char buf[RLS];
            int xx = fread(buf, RLS, 1, dump_fd);
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
         m_ptrSdr->wait(data, len); // len=total sample number = #i+#q
      }

      int ld = dc.process_iq(data, len, filter_type);
      fsk->process(data, ld);

      if (!dump_fd)
      {
         m_ptrSdr->done(len);
      }

      if (timeout && (time(0) - start > timeout))
      {
         break;
      }
   }
}
//-------------------------------------------------------------------------
