/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _INCLUDE_SDR_H
#define _INCLUDE_SDR_H

#include <string>
#include <vector>
#include <thread>

#include <rtl-sdr.h>

#include "utils/utils.h"
#include "utils/defines.h"

class sdr
{
public:
   sdr(int serial = 0, int dbg = 0, int dumpmode = 0, char *dumpfile = NULL);
   virtual ~sdr();

   void get_properties(std::string &paramVendor, std::string &paramProduct, std::string &parmSerial);
   int set_buffer_len(int paramBufferLength);
   int set_frequency(uint32_t paramFrequency);

   /**
    * set gain mode and value
    */
   int set_gain(eGainMode paramGainMaode, float paramGain);
   int set_ppm(int paramPPM);
   int set_samplerate(int paramSampleRate);

   int start();
   void stop();
   int wait(int16_t *&d, int &l);
   void done(int len);

//	virtual	handle_data();
   virtual void read_data(unsigned char const *const buf, uint32_t const &len);
   static int search_device(char *substr);

   volatile int wr_ptr, rd_ptr;

private:

   void read_thread(void);
   int nearest_gain(int paramGain);

   std::string m_Vendor;
   std::string m_Product;
   std::string m_Serial;

   std::string m_Logger;

   bool m_IsRunning;
   bool m_DebugMode;

   rtlsdr_dev_t *m_ptrRtlSdrDevice;

   std::thread *m_SdrReadThread;
   pthread_cond_t ready;
   pthread_mutex_t ready_m;

   int16_t *m_ptrBuffer;

   eGainMode m_GainMode;
   int m_GainValue;
   int m_FrequencyCorrection;

   int buffer_len;

   int cur_sr;

   uint32_t cur_frequ;

   FILE *m_FileDataDump;

};

#endif
