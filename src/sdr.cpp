/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>

 sdr.cpp -- wrapper around librtlsdr

 Parts taken from librtlsdr (rtl_fm.c)
 rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "sdr.h"

#define READ_TIMEOUT 5

//-------------------------------------------------------------------------
sdr::sdr(int dev_index, int _dbg, int dumpmode, char *dumpfile)
   :
   m_Logger("SDR : ")
{
   m_ptrBuffer = 0;
   wr_ptr = 0;
   rd_ptr = 0;
   m_SdrReadThread = NULL;
   m_FileDataDump = NULL;

   m_IsRunning = false;
   m_DebugMode = _dbg ? true : false;
   m_GainMode = eGainMode::eUnknown;

   m_ptrRtlSdrDevice = NULL;

   // dumpmode = -1 -> LOAD Data from File
   // dumpmode = 0 | 1 -> SDR Only | SAVE Data
   if (0 < dumpmode)
   {
      if (NULL != dumpfile)
      {
         // save the data
         m_FileDataDump = fopen(dumpfile, "wb");
         if (NULL == m_FileDataDump)
         {
            printf("(%s) %s : %s\n", "ERR", m_Logger.c_str(), "cant open dump file!");
            perror(dumpfile);
            exit(-1);
         }
      }
      else
      {
         printf("(%s) %s : %s\n", "WAR", m_Logger.c_str(), "cant save data, no file given!");
      }
   }

   set_buffer_len(2 * 1024 * 1024);
   m_Vendor = "";
   m_Product = "";
   m_Serial = "";

   char vendorc[256], productc[256], serialc[256];
   rtlsdr_get_device_usb_strings(dev_index, vendorc, productc, serialc);
   m_Vendor = vendorc;
   m_Product = productc;
   m_Serial = serialc;

   int r;
   int retryCount = 10;
   while (retryCount)
   {
      r = rtlsdr_open(&m_ptrRtlSdrDevice, dev_index);
      if (!r)
      {
         break;
      }

      fprintf(stderr, "cant open device, return code was :  %i\n", r);
      fprintf(stderr, "will retry (ten times), in one second!\n");
      retryCount -= 1;
      sleep(1);
   }
   // set frequency correction
   set_ppm(0);

   pthread_cond_init(&ready, NULL);
   pthread_mutex_init(&ready_m, NULL);
}
//-------------------------------------------------------------------------
sdr::~sdr(void)
{
   stop();
}
//-------------------------------------------------------------------------
int sdr::search_device(char *substr)
{
   int device_count, device, offset;
   char *s2;
   char vendor[256], product[256], serial[256], sum[770]; // summ needs to be 3*256 + X ...

   // get number of known devices
   device_count = rtlsdr_get_device_count();

   if (0 == device_count)
   {
      printf("No supported devices found.\n");
      return -1;
   }

   if (!strcmp(substr, "?"))
   {
      printf("Found %d device(s):\n", device_count);
   }

   for (int i = 0; i < device_count; i++)
   {
      rtlsdr_get_device_usb_strings(i, vendor, product, serial);
      if (!strcmp(substr, "?"))
      {
         printf("  %d:  %s, %s, SN%s\n", i, vendor, product, serial);
      }
      else
      {
         snprintf(sum, sizeof(sum), "%s %s SN%s", vendor, product, serial);
         if (strcasestr(sum, substr))
         {
            printf("match device index %i %s\n", i, sum);
            return i;
         }
      }
   }
   return -1;
}
//-------------------------------------------------------------------------
int sdr::start(void)
{
   if (NULL == m_ptrRtlSdrDevice)
   {
      printf("ERR : No Device");
      return -1;
   }

   rtlsdr_reset_buffer(m_ptrRtlSdrDevice);

   m_IsRunning = true;
   m_SdrReadThread = new std::thread(&sdr::read_thread, this);
   alarm(READ_TIMEOUT);
   return 0;
}
//-------------------------------------------------------------------------
void sdr::stop()
{
   if (NULL != m_SdrReadThread)
   {
      printf("(%s) %s : %s\n", "INF", m_Logger.c_str(), "shuting down SDR Thread");
      rtlsdr_cancel_async(m_ptrRtlSdrDevice);
      m_SdrReadThread->join();
      delete m_SdrReadThread;
      m_SdrReadThread = NULL;
   }

   if (NULL != m_FileDataDump)
   {
      printf("(%s) %s : %s\n", "INF", m_Logger.c_str(), "closing file handle");
      fclose(m_FileDataDump);
   }

   m_IsRunning = false;
}
//-------------------------------------------------------------------------
// l: number of samples
int sdr::set_buffer_len(int paramBufferLength)
{
   if (true == m_IsRunning)
   {
      return 0;
   }

   if (NULL != m_ptrBuffer)
   {
      free(m_ptrBuffer);
   }

   // alloc some stuff
   m_ptrBuffer = (int16_t*) malloc(sizeof(int16_t) * paramBufferLength);

   buffer_len = paramBufferLength;
   wr_ptr = 0;
   rd_ptr = 0;

   return 0;
}
//-------------------------------------------------------------------------
int sdr::set_frequency(uint32_t paramFrequency)
{
   int tRet = 0;
   cur_frequ = paramFrequency;

   if (NULL == m_ptrRtlSdrDevice)
   {
      printf("ERR : No Device");
      return -1;
   }

   tRet = rtlsdr_set_center_freq(m_ptrRtlSdrDevice, cur_frequ);
   if (true == m_DebugMode)
   {
      uint32_t tF = rtlsdr_get_center_freq(m_ptrRtlSdrDevice);
      printf("Frequency %.4lfMHz\n", (float) (tF / 1e6));
   }
   return tRet;
}
//-------------------------------------------------------------------------
void sdr::get_properties(std::string &v, std::string &p, std::string &s)
{
   v = m_Vendor;
   p = m_Product;
   s = m_Serial;
}

int sdr::nearest_gain(int paramGain)
{
   int err1, err2;

   int r = rtlsdr_set_tuner_gain_mode(m_ptrRtlSdrDevice, 1);
   if (r < 0)
   {
      fprintf(stderr, "WARNING: Failed to enable manual gain!\n");
      return r;
   }

   int count = rtlsdr_get_tuner_gains(m_ptrRtlSdrDevice, NULL);
   if (count <= 0)
   {
      fprintf(stderr, "WARNING: cant get tuners gain values!\n");
      return 0;
   }
   else
   {
      if (true == m_DebugMode)
      {
         printf("Number of GAIN : %i\n", count);
      }
   }

   int *tptrGains = (int*) malloc(sizeof(int) * count);
   count = rtlsdr_get_tuner_gains(m_ptrRtlSdrDevice, tptrGains);
   int nearest = tptrGains[0];
   for (int i = 0; i < count; i++)
   {
      err1 = abs(paramGain - nearest);
      err2 = abs(paramGain - tptrGains[i]);
      if (err2 < err1)
      {
         nearest = tptrGains[i];
      }
   }
   free(tptrGains);
   return nearest;
}
//-------------------------------------------------------------------------
int sdr::set_gain(eGainMode paramGainMaode, float paramGainValue)
{
   // set the gain mode
   m_GainMode = paramGainMaode;

   if (NULL == m_ptrRtlSdrDevice)
   {
      printf("ERR : No Device");
      return -1;
   }

   if (eGainMode::eAuto == m_GainMode)
   {
      if (true == m_DebugMode)
      {
         printf("(%s) %s : %s\n", "DBG", m_Logger.c_str(), "GAIN Mode -> AUTO");
      }
      rtlsdr_set_tuner_gain_mode(m_ptrRtlSdrDevice, 0);
   }
   else if (eGainMode::eUser == m_GainMode)
   {
      if (true == m_DebugMode)
      {
         printf("(%s) %s : %s\n", "DBG", m_Logger.c_str(), "GAIN Mode -> USER");
      }
      rtlsdr_set_tuner_gain_mode(m_ptrRtlSdrDevice, 1);

      // calc the best gain supported by tuner
      int tNearestGain = nearest_gain(paramGainValue * 10);
      rtlsdr_set_tuner_gain(m_ptrRtlSdrDevice, tNearestGain);
   }
   else
   {
      printf("(%s) %s : %s\n", "ERR", m_Logger.c_str(), "GAIN Mode -> UNKNOWN");
   }

   // get the gain
   m_GainValue = rtlsdr_get_tuner_gain(m_ptrRtlSdrDevice);
   if (true == m_DebugMode)
   {
      printf("(%s) %s : %s : %.1f\n", "DBG", m_Logger.c_str(), "GAIN Value", m_GainValue / 10.0);
   }

   return m_GainValue;
}
//-------------------------------------------------------------------------
int sdr::set_ppm(int paramFrequencyCorrection)
{
   if (NULL == m_ptrRtlSdrDevice)
   {
      printf("ERR : No Device");
      return -1;
   }
   int tRet = rtlsdr_set_freq_correction(m_ptrRtlSdrDevice, paramFrequencyCorrection);

   // get the correction
   m_FrequencyCorrection = rtlsdr_get_freq_correction(m_ptrRtlSdrDevice);
   if (true == m_DebugMode)
   {
      printf("(%s) %s : %s -> %i\n", "DBG", m_Logger.c_str(), "Freq. Corr!", m_FrequencyCorrection);
   }

   return tRet;
}
//-------------------------------------------------------------------------
int sdr::set_samplerate(int s)
{
   int tRet = -1;
   if (NULL == m_ptrRtlSdrDevice)
   {
      printf("(%s) %s : %s\n", "ERR", m_Logger.c_str(), "No Device!");
   }
   else
   {
      int tRet = rtlsdr_set_sample_rate(m_ptrRtlSdrDevice, s);
      if (true == m_DebugMode)
      {
         printf("Samplerate %i\n", s);
      }
      cur_sr = s;
   }
   return tRet;
}
//-------------------------------------------------------------------------

void sdr::read_data(unsigned char const *const buf, uint32_t const &len)
{
   int w = wr_ptr;

   if (NULL != m_FileDataDump)
   {
      // write data to dump file if it is set
      fwrite(buf, len, 1, m_FileDataDump);
   }

   for (uint32_t i = 0; i < len; i++)
   {
      m_ptrBuffer[w++] = ((int16_t)(buf[i]) - 128) << 6; // Scale to +-8192
      if (w >= buffer_len)
      {
         w = 0;
      }
   }
   wr_ptr = w;
   safe_cond_signal(&ready, &ready_m);
}
//-------------------------------------------------------------------------
static void callback(unsigned char *buf, uint32_t len, void *ctx)
{
   if (NULL == ctx)
   {
      printf("(%s) %s : %s\n", "ERR", "STATIC", "No valid SDR Ptr!");
   }
   else
   {
      // cast the "context"
      sdr *p = (sdr*) ctx;
      p->read_data(buf, len);
      alarm(READ_TIMEOUT);
   }
}
//-------------------------------------------------------------------------
void sdr::read_thread(void)
{
   if (true == m_DebugMode)
   {
      printf("(%s) %s : %s\n", "DBG", m_Logger.c_str(), "starting read thread");
   }
   rtlsdr_read_async(m_ptrRtlSdrDevice, callback, this, 0, buffer_len / 8);
}
//-------------------------------------------------------------------------
int sdr::wait(int16_t *&paramInData, int &len)
{
   safe_cond_wait(&ready, &ready_m);

   paramInData = m_ptrBuffer + rd_ptr;
   if (rd_ptr < wr_ptr)
   {
      len = wr_ptr - rd_ptr;
   }
   else
   {
      len = buffer_len - rd_ptr; // ignore wraparound, defer to next read
   }
   return 0;
}
//-------------------------------------------------------------------------
void sdr::done(int len)
{
   int r = rd_ptr + len;
   if (r >= buffer_len)
   {
      r = r - buffer_len;
   }
   rd_ptr = r;
}
//-------------------------------------------------------------------------
