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
{
   buffer = 0;
   wr_ptr = 0;
   rd_ptr = 0;
   r_thread = NULL;
   dump_fd = NULL;

   m_IsRunning = false;
   m_DebugMode = _dbg ? true : false;
   m_GainMode = eGainMode::eUnknown;

   m_ptrRtlSdrDevice = NULL;

   if (dumpmode > 0 && dumpfile)
   {
      dump_fd = fopen(dumpfile, "wb");
      if (!dump_fd)
      {
         perror(dumpfile);
         exit(-1);
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
      fprintf(stderr, "will retry (ten times), in short time!\n");
      retryCount -= 1;
      sleep(2);
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
   r_thread = new std::thread(&sdr::read_thread, this);
   alarm(READ_TIMEOUT);
   return 0;
}
//-------------------------------------------------------------------------
int sdr::stop(void)
{
   if (NULL != r_thread)
   {
      rtlsdr_cancel_async(m_ptrRtlSdrDevice);
      r_thread->join();
      delete r_thread;
      r_thread = NULL;
   }

   m_IsRunning = false;
   return 0;
}
//-------------------------------------------------------------------------
// l: number of samples
int sdr::set_buffer_len(int paramBufferLength)
{
   if (true == m_IsRunning)
   {
      return 0;
   }

   if (NULL != buffer)
   {
      free(buffer);
   }

   // alloc some stuff
   buffer = (int16_t*) malloc(sizeof(int16_t) * paramBufferLength);

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
         printf("GAIN Mode  : AUTO \n");
      }
      rtlsdr_set_tuner_gain_mode(m_ptrRtlSdrDevice, 0);
   }
   else if (eGainMode::eUser == m_GainMode)
   {
      if (true == m_DebugMode)
      {
         printf("GAIN Mode  : USER\n");
      }
      rtlsdr_set_tuner_gain_mode(m_ptrRtlSdrDevice, 1);

      // calc the best gain supported by tuner
      int tNearestGain = nearest_gain(paramGainValue * 10);
      rtlsdr_set_tuner_gain(m_ptrRtlSdrDevice, tNearestGain);
   }
   else
   {
      printf("ERR : gein Mode unknown");
      return -1;
   }

   // get the gain
   m_GainValue = rtlsdr_get_tuner_gain(m_ptrRtlSdrDevice);
   if (true == m_DebugMode)
   {
      printf("GAIN Value : %.1f\n", m_GainValue / 10.0);
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
      printf("Freq. Corr : %i\n", m_FrequencyCorrection);
   }

   return tRet;
}
//-------------------------------------------------------------------------
int sdr::set_samplerate(int s)
{
   if (NULL == m_ptrRtlSdrDevice)
   {
      printf("ERR : No Device");
      return -1;
   }

   int tRet = rtlsdr_set_sample_rate(m_ptrRtlSdrDevice, s);
   if (true == m_DebugMode)
   {
      printf("Samplerate %i\n", s);
   }
   cur_sr = s;
   return tRet;
}
//-------------------------------------------------------------------------

void sdr::read_data(unsigned char *buf, uint32_t len)
{
   //printf("Got %i\n", len);

   int w = wr_ptr;

   if (dump_fd)
   {
      fwrite(buf, len, 1, dump_fd);
   }

   for (uint32_t i = 0; i < len; i++)
   {
      buffer[w++] = ((int16_t)(buf[i]) - 128) << 6; // Scale to +-8192
      if (w >= buffer_len)
      {
         w = 0;
      }
   }
   wr_ptr = w;
   safe_cond_signal(&ready, &ready_m);
}
//-------------------------------------------------------------------------
static void sdr_read_callback(unsigned char *buf, uint32_t len, void *ctx)
{
   if (NULL == ctx)
   {
      // no valid ptr
      printf("ERR : No valid SDR Ptr!");
      return;
   }

   // cast the "context"
   sdr *this_ptr = (sdr*) ctx;
   this_ptr->read_data(buf, len);
   alarm(READ_TIMEOUT);
}
//-------------------------------------------------------------------------
void sdr::read_thread(void)
{
   if (true == m_DebugMode)
   {
      printf("START READ THREAD\n");
   }

   rtlsdr_read_async(m_ptrRtlSdrDevice, sdr_read_callback, this, 0, buffer_len / 8);
}
//-------------------------------------------------------------------------
int sdr::wait(int16_t *&d, int &len)
{
   //printf("SDR wait\n");
   safe_cond_wait(&ready, &ready_m);
   d = buffer + rd_ptr;
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
