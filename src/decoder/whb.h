#ifndef _INCLUDE_TFA5H
#define _INCLUDE_TFA5H

// Base Classes
#include "decoder/decoder.h"

// Helper
#include "utils/crc/crc32.h"
#include "utils/dsp_stuff.h"

class whb_decoder: public decoder
{

public:
   whb_decoder(sensor_e type = TFA_WHB);
   virtual ~whb_decoder();
   void store_bit(int bit);
   void flush(int rssi, int offset = 0);

private:
   double cvt_temp(uint16_t raw);
   void decode_02(uint8_t *msg, uint64_t id, int rssi, int offset); // temp
   void decode_03(uint8_t *msg, uint64_t id, int rssi, int offset); // temp/hum
   void decode_04(uint8_t *msg, uint64_t id, int rssi, int offset); // temp/hum/water
   void decode_06(uint8_t *msg, uint64_t id, int rssi, int offset); // temp/hum + temp (TFA 30.3304.02)
   void decode_07(uint8_t *msg, uint64_t id, int rssi, int offset); // Station MA10410 (TFA 35.1147.01)
   void decode_08(uint8_t *msg, uint64_t id, int rssi, int offset); // rain
   void decode_0b(uint8_t *msg, uint64_t id, int rssi, int offset); // wind
   void decode_10(uint8_t *msg, uint64_t id, int rssi, int offset); // door
   void decode_11(uint8_t *msg, uint64_t id, int rssi, int offset); // 4 Thermo-hygro-sensors (TFA 30.3060.01)
   void decode_12(uint8_t *msg, uint64_t id, int rssi, int offset); // Humidity guard/cosy radar (TFA 30.5043.01)

   uint32_t sr;
   int sr_cnt;
   int snum;
   crc32 *crc;

   // Decoding
   int last_bit;
   int psk, last_psk;
   int nrzs;
   uint32_t lfsr;
};

#endif
